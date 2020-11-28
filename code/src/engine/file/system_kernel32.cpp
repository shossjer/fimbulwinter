#include "config.h"

#if FILE_USE_KERNEL32

#include "system.hpp"

#include "core/container/Collection.hpp"
#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/functional/utility.hpp"
#include "utility/string.hpp"

#include <Windows.h>

namespace
{
	engine::Asset make_asset(utility::string_units_utfw filepath)
	{
		return engine::Asset(reinterpret_cast<const char *>(filepath.data()), filepath.size() * sizeof(wchar_t));
	}
}

namespace
{
	struct Path
	{
		utility::heap_string_utfw filepath;
		ext::ssize root;

		explicit Path(utility::heap_string_utfw && filepath)
			: filepath(std::move(filepath))
			, root(this->filepath.size())
		{}

		explicit Path(utility::heap_string_utfw && filepath, ext::ssize root)
			: filepath(std::move(filepath))
			, root(root)
		{}
	};

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	struct ReadWatch
	{
		utility::heap_string_utfw filepath;
		engine::file::read_callback * callback;
		utility::any data;

		FILETIME last_write_time;
	};

	struct ScanWatch
	{
		engine::file::scan_callback * callback;
		utility::any data;
	};

	struct WatchData
	{
		OVERLAPPED overlapped;

		utility::heap_string_utf8 subdir;
		engine::file::flags flags; // 0-bit recurse
		utility::heap_vector<utility::heap_string_utf8, ReadWatch> reads;
		utility::heap_vector<engine::Asset, ScanWatch> scans;

		std::aligned_storage_t<1024, alignof(DWORD)> buffer; // arbitrary

		explicit WatchData(HANDLE hDirectory, utility::heap_string_utf8 && subdir, engine::file::flags flags)
			: flags(flags)
			, subdir(std::move(subdir))
		{
			overlapped.hEvent = hDirectory;
		}

		bool read()
		{
			DWORD notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
//#if MODE_DEBUG
//			notify_filter |= FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
//#endif
			return debug_verify(
				::ReadDirectoryChangesW(
					overlapped.hEvent,
					&buffer,
					sizeof buffer,
					flags & engine::file::flags::RECURSE_DIRECTORIES ? TRUE : FALSE,
					notify_filter,
					nullptr,
					&overlapped,
					Completion) != FALSE,
				"failed with last error ", ::GetLastError());
		}
	};
	static_assert(std::is_standard_layout<WatchData>::value, "WatchData must be pointer-interconvertible with OVERLAPPED!");
	static_assert(offsetof(WatchData, overlapped) == 0, "");

	utility::heap_vector<engine::Asset, WatchData *> watches;

	WatchData * start_watch(engine::Asset id, const Path & path, engine::file::flags flags)
	{
		auto watch_it = ext::find_if(watches, fun::first == id);
		if (watch_it != watches.end())
			return *watch_it.second;

		debug_printline("starting watch \"", utility::heap_narrow<utility::encoding_utf8>(path.filepath), "\"");
		HANDLE hDirectory = ::CreateFileW(
			path.filepath.data(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);
		if (!debug_verify(hDirectory != INVALID_HANDLE_VALUE, "CreateFileW failed with last error ", ::GetLastError()))
			return nullptr;

		utility::heap_string_utf8 subdir;
		if (!debug_verify(utility::try_narrow(utility::string_points_utfw(path.filepath.data() + path.root, path.filepath.data() + path.filepath.size()), subdir)))
		{
			debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			return nullptr;
		}

		WatchData * const data = new WatchData(hDirectory, std::move(subdir), flags);
		if (!debug_assert(data))
		{
			debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			return nullptr;
		}

		if (!debug_verify(watches.try_emplace_back(id, data)))
		{
			delete data;
			debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			return nullptr;
		}

		if (!data->read())
		{
			ext::pop_back(watches);
			delete data;
			debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			return nullptr;
		}
		return data;
	}

	bool stop_watch(decltype(watches.begin()) watch_it)
	{
		if (!debug_assert(watch_it != watches.end()))
			return false;

		if (::CancelIoEx((*watch_it.second)->overlapped.hEvent, &(*watch_it.second)->overlapped) == FALSE)
		{
			const auto error = ::GetLastError();
			debug_verify(error == ERROR_NOT_FOUND, "CancelIoEx failed with last error ", error);
			return false;
		}
		watches.erase(watch_it);

		return true;
	}

	utility::heap_vector<char> changes_buffer;
	HANDLE hEvent = nullptr;

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		WatchData * const watch = reinterpret_cast<WatchData *>(lpOverlapped); // note fine since pointer-interconvertible

		if (dwErrorCode != 0)
		{
			debug_assert(dwErrorCode == ERROR_OPERATION_ABORTED);
			debug_assert(0 == dwNumberOfBytesTransfered);
		}
		else if (dwNumberOfBytesTransfered == 0)
		{
			debug_fail("scan needed"); // todo

			if (watch->read())
				return;
		}
		else
		{
			const DWORD size = (dwNumberOfBytesTransfered + (alignof(DWORD) - 1)) & ~(alignof(DWORD) - 1);
			debug_assert(size <= sizeof watch->buffer);
			const DWORD chunk_size = sizeof(LPVOID) + sizeof(DWORD) + size;

			debug_assert(changes_buffer.size() % alignof(DWORD) == 0);
			if (debug_verify(changes_buffer.try_reserve(changes_buffer.size() + chunk_size)))
			{
				changes_buffer.push_back(utility::no_failure, reinterpret_cast<const char *>(&watch), reinterpret_cast<const char *>(&watch) + sizeof(LPVOID));
				changes_buffer.push_back(utility::no_failure, reinterpret_cast<const char *>(&chunk_size), reinterpret_cast<const char *>(&chunk_size) + sizeof(DWORD));
				changes_buffer.push_back(utility::no_failure, reinterpret_cast<const char *>(&watch->buffer), reinterpret_cast<const char *>(&watch->buffer) + size);

				debug_verify(::SetEvent(hEvent) != FALSE, "failed with last error ", ::GetLastError());
			}
			if (watch->read())
				return;
		}

		debug_printline("deleting watch \"", watch->subdir, "\"");
		delete watch;
	}

	struct Directory
	{
		Path path;

		ext::ssize share_count;

		explicit Directory(utility::heap_string_utfw && filepath)
			: path(std::move(filepath))
			, share_count(0)
		{}

		explicit Directory(utility::heap_string_utfw && filepath, ext::ssize root)
			: path(std::move(filepath), root)
			, share_count(0)
		{}
	};

	struct TemporaryDirectory
	{
		Path path;

		explicit TemporaryDirectory(utility::heap_string_utfw && filepath)
			: path(std::move(filepath))
		{}
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<Directory>,
		utility::heap_storage<TemporaryDirectory>
	>
	directories;

	const Path & get_path(decltype(directories)::const_iterator it)
	{
		struct
		{
			const Path & operator () (const Directory & x) { return x.path; }
			const Path & operator () (const TemporaryDirectory & x) { return x.path; }
		}
		visitor;

		return directories.call(it, visitor);
	}

	struct Alias
	{
		engine::Asset directory;
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<Alias>
	>
	aliases;

	void purge_temporary_directory(utility::heap_string_utfw & filepath)
	{
		if (!debug_assert(back(filepath) == L'\\'))
			return;

		filepath.data()[filepath.size() - 1] = L'\0';

		debug_printline("removing temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

		SHFILEOPSTRUCTW op =
		{
			nullptr,
			FO_DELETE,
			filepath.data(),
			nullptr,
			FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
			FALSE,
			nullptr,
			nullptr
		};

		// todo this command is nasty
		const int ret = ::SHFileOperationW(&op);
		debug_verify((ret == 0 && op.fAnyOperationsAborted == FALSE), "SHFileOperationW failed with return value ", ret);
	}

	void scan_directory(const Path & path, bool recurse, utility::heap_string_utfw & files)
	{
		debug_printline("scanning directory \"", utility::heap_narrow<utility::encoding_utf8>(path.filepath), "\"");

		utility::heap_vector<utility::heap_string_utfw> subdirs;
		if (!debug_verify(subdirs.try_emplace_back()))
			return; // error

		utility::heap_string_utfw pattern;
		if (!debug_verify(pattern.try_append(path.filepath)))
			return; // error

		while (!ext::empty(subdirs))
		{
			auto subdir = ext::back(std::move(subdirs));
			ext::pop_back(subdirs);

			if (!debug_verify(pattern.try_append(subdir)))
				return; // error

			if (!debug_verify(pattern.try_push_back(L'*')))
				return; // error

			WIN32_FIND_DATAW data;

			HANDLE hFile = ::FindFirstFileW(pattern.data(), &data);
			if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "FindFirstFileW failed with last error ", ::GetLastError()))
				return; // error

			do
			{
				if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if (recurse)
					{
						if (data.cFileName[0] == L'.' && data.cFileName[1] == L'\0')
							continue;

						if (data.cFileName[0] == L'.' && data.cFileName[1] == L'.' && data.cFileName[2] == L'\0')
							continue;

						if (!debug_verify(subdirs.try_emplace_back()))
							return; // error

						auto & dir = ext::back(subdirs);

						if (!debug_verify(dir.try_append(subdir)))
							return; // error

						if (!debug_verify(dir.try_append(static_cast<const wchar_t *>(data.cFileName))))
							return; // error

						if (!debug_verify(dir.try_push_back(L'/')))
							return; // error
					}
				}
				else
				{
					if (!debug_verify(files.try_push_back(L'+')))
						return; // error

					if (!debug_verify(files.try_append(subdir)))
						return; // error

					if (!debug_verify(files.try_append(static_cast<const wchar_t *>(data.cFileName))))
						return; // error

					if (!debug_verify(files.try_push_back(L';')))
						return; // error
				}
			}
			while (::FindNextFileW(hFile, &data) != FALSE);

			const auto error = ::GetLastError();
			if (!debug_verify(error == ERROR_NO_MORE_FILES, "FindNextFileW failed with last error ", error))
				return; // error

			debug_verify(::FindClose(hFile) != FALSE, "failed with last error ", ::GetLastError());

			pattern.reduce(subdir.size() + 1); // * is one character
		}

		if (!empty(files))
		{
			files.reduce(1); // trailing ;
		}
	}

	bool read_file(utility::string_units_utfw filepath, utility::heap_string_utf8 && filepath_utf8, engine::file::read_callback * callback, utility::any & data, FILETIME * last_write_time = nullptr)
	{
		HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", filepath_utf8, "\" failed with last error ", ::GetLastError()))
			return false;

		if (last_write_time)
		{
			BY_HANDLE_FILE_INFORMATION by_handle_file_information;
			if (debug_verify(::GetFileInformationByHandle(hFile, &by_handle_file_information) != 0, "GetFileInformationByHandle failed with last error ", ::GetLastError()))
			{
				if (by_handle_file_information.ftLastWriteTime.dwHighDateTime == last_write_time->dwHighDateTime &&
				    by_handle_file_information.ftLastWriteTime.dwLowDateTime == last_write_time->dwLowDateTime)
				{
					debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

					return true;
				}
				*last_write_time = by_handle_file_information.ftLastWriteTime;
			}
		}

		core::ReadStream stream(
			[](void * dest, ext::usize n, void * data)
		{
			HANDLE hFile = reinterpret_cast<HANDLE>(data);

			DWORD read;
			if (!debug_verify(::ReadFile(hFile, dest, debug_cast<DWORD>(n), &read, nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				return ext::ssize(-1);

			return ext::ssize(read);
		},
			reinterpret_cast<void *>(hFile),
			std::move(filepath_utf8));

		callback(std::move(stream), data);

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

		return true;
	}

	HANDLE hThread = nullptr;
	bool active = false;

	DWORD WINAPI file_watch(LPVOID /*arg*/)
	{
		while (active)
		{
			const auto ret = ::WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
			if (ret == WAIT_IO_COMPLETION)
				continue;

			if (!debug_verify(ret != WAIT_FAILED, "WaitForSingleObjectEx failed with last error ", ::GetLastError()))
				break;

			if (!debug_assert(ret == WAIT_OBJECT_0))
				break;

			const char * buffer = reinterpret_cast<const char *>(changes_buffer.data());
			const char * buffer_end = buffer + changes_buffer.size();
			while (buffer != buffer_end)
			{
				WatchData * watch_data;
				std::copy(buffer, buffer + sizeof(LPVOID), reinterpret_cast<char *>(&watch_data));
				const std::size_t chunk_size = *reinterpret_cast<const DWORD *>(buffer + sizeof(LPVOID));

				utility::heap_string_utf8 files;

				std::size_t offset = sizeof(LPVOID) + sizeof(DWORD);
				bool chunk_done = false;
				while (!chunk_done)
				{
					debug_assert(offset % alignof(DWORD) == 0);

					const FILE_NOTIFY_INFORMATION * const info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(buffer + offset);
					debug_assert(info->FileNameLength % sizeof(wchar_t) == 0);

					chunk_done = info->NextEntryOffset == 0;
					offset += info->NextEntryOffset;

					const utility::string_points_utfw filename(info->FileName, info->FileNameLength / sizeof(wchar_t));

					utility::heap_string_utf8 match_name;
					if (!debug_verify(match_name.try_append(watch_data->subdir)))
						continue;
					if (!debug_verify(utility::try_narrow(filename, match_name)))
						continue;

					// todo replace function
					auto begin_name = match_name.begin() + watch_data->subdir.size();
					const auto end_name = match_name.end();
					while (true)
					{
						const auto slash = find(begin_name, end_name, '\\');
						if (slash == end_name)
							break;

						*slash = '/';

						begin_name = slash + 1;
					}

					switch (info->Action)
					{
					case FILE_ACTION_ADDED:
						debug_printline("change FILE_ACTION_ADDED: ", match_name);
						if (!debug_verify(files.try_push_back('+')))
							continue;
						if (!debug_verify(files.try_append(match_name)))
							continue;
						if (!debug_verify(files.try_push_back(';')))
							continue;
						break;
					case FILE_ACTION_REMOVED:
						debug_printline("change FILE_ACTION_REMOVED: ", match_name);
						if (!debug_verify(files.try_push_back('-')))
							continue;
						if (!debug_verify(files.try_append(match_name)))
							continue;
						if (!debug_verify(files.try_push_back(';')))
							continue;
						break;
					case FILE_ACTION_MODIFIED:
						debug_printline("change FILE_ACTION_MODIFIED: ", match_name);
						{
							auto read_begin = watch_data->reads.begin();
							auto read_end = watch_data->reads.end();
							for (; read_begin != read_end; ++read_begin)
							{
								if (*read_begin.first == utility::string_units_utf8(match_name.data() + watch_data->subdir.size(), match_name.data() + match_name.size()))
								{
									read_file(read_begin.second->filepath, std::move(match_name), read_begin.second->callback, read_begin.second->data, &read_begin.second->last_write_time);
								}
							}
						}
						break;
					case FILE_ACTION_RENAMED_OLD_NAME:
						debug_printline("change FILE_ACTION_RENAMED_OLD_NAME: ", match_name);
						break;
					case FILE_ACTION_RENAMED_NEW_NAME:
						debug_printline("change FILE_ACTION_RENAMED_NEW_NAME: ", match_name);
						break;
					default:
						debug_unreachable("unknown action ", info->Action);
					}
				}

				debug_printline(files);

				if (!empty(files))
				{
					auto scan_begin = watch_data->scans.begin();
					auto scan_end = watch_data->scans.end();
					for (; scan_begin != scan_end; ++scan_begin)
					{
						scan_begin.second->callback(*scan_begin.first, utility::heap_string_utf8(files, 0, files.size() - 1), scan_begin.second->data);
					}
				}
				buffer += chunk_size;
			}

			debug_verify(::ResetEvent(hEvent) != FALSE, "failed with last error ", ::GetLastError());
			changes_buffer.clear();
		}

		debug_assert(ext::empty(watches));

		return 0;
	}

	utility::const_string_iterator<utility::boundary_unit_utf8> check_filepath(utility::const_string_iterator<utility::boundary_unit_utf8> begin, utility::const_string_iterator<utility::boundary_unit_utf8> end)
	{
		// todo disallow windows absolute paths

		// todo report \\ as error

		while (true)
		{
			const auto slash = find(begin, end, u8'/');
			if (slash == begin)
				return slash; // note this disallows linux absolute paths

			if (*begin == u8'.')
			{
				auto next = begin + 1;
				if (next == slash)
					return begin; // disallow .

				if (*next == u8'.')
				{
					++next;
					if (next == slash)
						return begin; // disallow ..
				}
			}
			if (slash == end)
				break;

			begin = slash + 1;
		}
		return end;
	}

	bool validate_filepath(utility::string_units_utf8 str)
	{
		return check_filepath(str.begin(), str.end()) == str.end();
	}

	struct RegisterDirectory
	{
		engine::Asset alias;
		engine::Asset parent;
		utility::heap_string_utf8 filepath;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<RegisterDirectory> data(reinterpret_cast<RegisterDirectory *>(Parameter));
			RegisterDirectory & x = *data;

			if (!debug_verify(find(aliases, x.alias) == aliases.end()))
				return; // error

			const auto parent_alias_it = find(aliases, x.parent);
			if (!debug_verify(parent_alias_it != aliases.end()))
				return; // error

			if (!debug_verify(validate_filepath(x.filepath)))
				return; // error

			const auto parent_directory_it = find(directories, aliases.get<Alias>(parent_alias_it)->directory);
			if (!debug_assert(parent_directory_it != directories.end()))
				return;

			const auto & path = get_path(parent_directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(path.filepath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			if (back(filepath) != L'/')
			{
				if (!debug_verify(filepath.try_push_back(L'/')))
					return; // error
			}

			const auto directory_asset = make_asset(filepath);
			const auto directory_it = find(directories, directory_asset);
			if (directory_it != directories.end())
			{
				const auto directory_ptr = directories.get<Directory>(directory_it);
				if (!debug_assert(directory_ptr))
					return;

				directory_ptr->share_count++;
			}
			else
			{
				const auto cpy = path.root;
				if (!debug_verify(directories.emplace<Directory>(directory_asset, std::move(filepath), cpy)))
					return; // error
			}

			if (!debug_verify(aliases.emplace<Alias>(x.alias, directory_asset)))
				return; // error, todo rollback
		}
	};

	struct RegisterTemporaryDirectory
	{
		engine::Asset alias;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<RegisterTemporaryDirectory> data(reinterpret_cast<RegisterTemporaryDirectory *>(Parameter));
			RegisterTemporaryDirectory & x = *data;

			utility::static_string_utfw<(MAX_PATH + 1 + 1)> temppath(MAX_PATH + 1); // GetTempPathW returns at most (MAX_PATH + 1) (not counting null terminator)
			if (!debug_verify(::GetTempPathW(MAX_PATH + 1 + 1, temppath.data()) != 0, "failed with last error ", ::GetLastError()))
				return;

			// todo resize temppath

			utility::static_string_utfw<MAX_PATH> tempfilepath(MAX_PATH - 1); // GetTempFileNameW returns at most MAX_PATH (including null terminator)
			if (!debug_verify(::GetTempFileNameW(temppath.data(), L"unn", 0, tempfilepath.data()) != 0, "failed with last error ", ::GetLastError()))
				return;

			// todo resize filepath

			if (!debug_verify(::DeleteFileW(tempfilepath.data()) != FALSE, "failed with last error ", ::GetLastError()))
				return;

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(static_cast<const wchar_t *>(tempfilepath.data()))))
				return; // error

			if (!debug_verify(::CreateDirectoryW(filepath.data(), nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				return;

			debug_printline("created temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

			if (!debug_verify(filepath.try_push_back(L'\\')))
				return; // error

			const auto directory_asset = make_asset(filepath);
			if (!debug_verify(directories.emplace<TemporaryDirectory>(directory_asset, std::move(filepath))))
				return; // error

			if (!debug_verify(aliases.emplace<Alias>(x.alias, directory_asset)))
				return; // error, todo rollback
		}
	};

	struct UnregisterDirectory
	{
		engine::Asset alias;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<UnregisterDirectory> data(reinterpret_cast<UnregisterDirectory *>(Parameter));
			UnregisterDirectory & x = *data;

			const auto alias_it = find(aliases, x.alias);
			if (!debug_verify(alias_it != aliases.end(), "alias does not exist"))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(directories, alias_ptr->directory);
			if (!debug_assert(directory_it != directories.end()))
				return;

			struct
			{
				bool operator () (Directory & x) { x.share_count--; return x.share_count < 0; }
				bool operator () (engine::Asset debug_expression(directory), TemporaryDirectory & x)
				{
					debug_expression(const auto watch_it = ext::find_if(watches, fun::first == directory));
					if (!debug_assert(watch_it == watches.end(), "Watches need to be removed before directories"))
					{
						debug_expression(stop_watch(watch_it));
					}

					purge_temporary_directory(x.path.filepath); return true;
				}
			}
			detach_alias;

			const auto directory_is_unused = directories.call(directory_it, detach_alias);
			if (directory_is_unused)
			{
				debug_expression(const auto watch_it = ext::find_if(watches, fun::first == alias_ptr->directory));
				if (!debug_assert(watch_it == watches.end(), "Watches need to be removed before directories"))
				{
					debug_expression(stop_watch(watch_it));
				}

				directories.erase(directory_it);
			}

			aliases.erase(alias_it);
		}
	};

	struct Read
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
		engine::file::read_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Read> data(reinterpret_cast<Read *>(Parameter));
			Read & x = *data;

			const auto alias_it = find(aliases, x.directory);
			if (!debug_verify(alias_it != aliases.end()))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(directories, alias_ptr->directory);
			if (!debug_assert(directory_it != directories.end()))
				return;

			const auto & path = get_path(directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(path.filepath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			utility::heap_string_utf8 filepath_utf8;
			if (!debug_verify(utility::try_narrow(utility::string_points_utfw(filepath.data() + path.root, filepath.data() + filepath.size()), filepath_utf8)))
				return; // error

			// todo if x.filepath contains slashes the RECURSE_DIRECTORIES flag must be set or else there are inconsistencies with watch
			if (!read_file(filepath, std::move(filepath_utf8), x.callback, x.data))
				return; // error

			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				WatchData * const watch_data = start_watch(alias_ptr->directory, path, x.mode);
				debug_verify(watch_data->reads.try_emplace_back(/*x.directory, */std::move(x.filepath), ReadWatch{std::move(filepath), x.callback, std::move(x.data), FILETIME{}}));
			}
		}
	};

	struct RemoveDirectoryWatch
	{
		engine::Asset directory;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<RemoveDirectoryWatch> data(reinterpret_cast<RemoveDirectoryWatch *>(Parameter));
			RemoveDirectoryWatch & x = *data;

			const auto alias_it = find(aliases, x.directory);
			if (!debug_verify(alias_it != aliases.end()))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto watch_it = ext::find_if(watches, fun::first == alias_ptr->directory);
			if (!debug_verify(watch_it != watches.end()))
				return; // error

			const auto scan_it = ext::find_if((*watch_it.second)->scans, fun::first == x.directory);
			if (!debug_verify(scan_it != (*watch_it.second)->scans.end()))
				return; // error

			(*watch_it.second)->scans.erase(scan_it);

			if (ext::empty((*watch_it.second)->reads) && ext::empty((*watch_it.second)->scans))
			{
				stop_watch(watch_it);
			}
		}
	};

	struct RemoveFileWatch
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<RemoveFileWatch> data(reinterpret_cast<RemoveFileWatch *>(Parameter));
			RemoveFileWatch & x = *data;

			const auto alias_it = find(aliases, x.directory);
			if (!debug_verify(alias_it != aliases.end()))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto watch_it = ext::find_if(watches, fun::first == alias_ptr->directory);
			if (!debug_verify(watch_it != watches.end()))
				return; // error

			const auto read_it = ext::find_if((*watch_it.second)->reads, fun::first == x.filepath);
			if (!debug_verify(read_it != (*watch_it.second)->reads.end()))
				return; // error

			(*watch_it.second)->reads.erase(read_it);

			if (ext::empty((*watch_it.second)->reads) && ext::empty((*watch_it.second)->scans))
			{
				stop_watch(watch_it);
			}
		}
	};

	struct Scan
	{
		engine::Asset directory;
		engine::file::scan_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Scan> data(reinterpret_cast<Scan *>(Parameter));
			Scan & x = *data;

			const auto alias_it = find(aliases, x.directory);
			if (!debug_verify(alias_it != aliases.end()))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(directories, alias_ptr->directory);
			if (!debug_assert(directory_it != directories.end()))
				return;

			const auto & path = get_path(directory_it);

			utility::heap_string_utfw files;
			scan_directory(path, static_cast<bool>(x.mode & engine::file::flags::RECURSE_DIRECTORIES), files);

			utility::heap_string_utf8 files_utf8;
			if (!debug_verify(utility::try_narrow(files, files_utf8)))
				return; // error

			x.callback(x.directory, std::move(files_utf8), x.data);

			// todo filesystem changes between the scan and the start of watch will not be detected
			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				WatchData * const watch_data = start_watch(alias_ptr->directory, path, x.mode);
				debug_verify(watch_data->scans.try_emplace_back(x.directory, ScanWatch{x.callback, std::move(x.data)}));
			}
		}
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Write> data(reinterpret_cast<Write *>(Parameter));
			Write & x = *data;

			const auto alias_it = find(aliases, x.directory);
			if (!debug_verify(alias_it != aliases.end()))
				return; // error

			const auto alias_ptr = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(directories, alias_ptr->directory);
			if (!debug_assert(directory_it != directories.end()))
				return;

			const auto & path = get_path(directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(path.filepath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			if (x.mode & engine::file::flags::CREATE_DIRECTORIES)
			{
				auto begin = filepath.begin() + path.root;
				const auto end = filepath.end();

				while (true)
				{
					const auto slash = find(begin, end, L'/');
					if (slash == end)
						break;

					*slash = L'\0';

					if (!debug_verify(CreateDirectoryW(filepath.data(), nullptr) != 0, ::GetLastError()))
						return; // error

					*slash = L'/';

					begin = slash + 1;
				}
			}

			DWORD dwCreationDisposition = CREATE_NEW;
			DWORD dwDesiredAccess = GENERIC_WRITE;
			if (x.mode & engine::file::flags::APPEND_EXISTING)
			{
				dwCreationDisposition = OPEN_ALWAYS;
				dwDesiredAccess = FILE_APPEND_DATA;
			}
			else if (x.mode & engine::file::flags::OVERWRITE_EXISTING)
			{
				dwCreationDisposition = CREATE_ALWAYS;
			}
			HANDLE hFile = ::CreateFileW(filepath.data(), dwDesiredAccess, 0, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				debug_verify(::GetLastError() == ERROR_FILE_EXISTS, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"failed with last error ", ::GetLastError());
				return;
			}

			utility::heap_string_utf8 filepath_utf8;
			if (!debug_verify(utility::try_narrow(utility::string_points_utfw(filepath.data() + path.root, filepath.data() + filepath.size()), filepath_utf8)))
				return; // error

			core::WriteStream stream(
				[](const void * src, ext::usize n, void * data)
			{
				HANDLE hFile = reinterpret_cast<HANDLE>(data);

				DWORD written;
				if (!debug_verify(::WriteFile(hFile, src, debug_cast<DWORD>(n), &written, nullptr) != FALSE, "failed with last error ", ::GetLastError()))
					return ext::ssize(-1);

				return ext::ssize(written);
			},
				reinterpret_cast<void *>(hFile),
				std::move(filepath_utf8));

			x.callback(std::move(stream), std::move(x.data));

			debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());
		}
	};

	struct Terminate
	{
		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Terminate> data(reinterpret_cast<Terminate *>(Parameter));

			for (auto watch_it = watches.begin(); watch_it != watches.end(); ++watch_it)
			{
				stop_watch(watch_it);
			}

			active = false;
		}
	};

	template <typename T, typename ...Ps>
	bool try_queue_apc(Ps && ...ps)
	{
		// todo raw allocations are sketchy
		T * const data = new T{std::forward<Ps>(ps)...};

		// todo Windows Server 2003 and Windows XP: There are no
		// error values defined for this function that can be
		// retrieved by calling GetLastError.
		//
		// https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-queueuserapc
		if (!debug_verify(::QueueUserAPC(T::Callback, hThread, reinterpret_cast<ULONG_PTR>(data)) != FALSE, "failed with last error ", ::GetLastError()))
		{
			delete data;
			return false;
		}
		return true;
	}
}

namespace engine
{
	namespace file
	{
		directory directory::working_directory()
		{
			directory dir;

			const auto len = GetCurrentDirectoryW(0, nullptr); // including null
			if (!debug_verify(len != 0, ::GetLastError()))
				return dir;

			if (!debug_verify(dir.filepath_.try_resize(len)))
				return dir;

			if (!debug_verify(GetCurrentDirectoryW(len, dir.filepath_.data()) != 0, ::GetLastError()))
				return dir;

			dir.filepath_.data()[len - 1] = L'\\';

			return dir;
		}

		system::~system()
		{
			if (!debug_verify(hThread != nullptr))
				return;

			if (!debug_assert(active != false))
				return;

			if (!debug_assert(hEvent != nullptr))
				return;

			if (!try_queue_apc<Terminate>())
			{
				// todo this could cause memory leaks due to the raw allocations for the apc queue
				debug_verify(::TerminateThread(hThread, DWORD(-1)) != FALSE, "failed with last error ", ::GetLastError());
			}

			debug_verify(::WaitForSingleObject(hThread, INFINITE) != WAIT_FAILED, "failed with last error ", ::GetLastError());
			hThread = nullptr;

			debug_verify(::CloseHandle(hEvent) != FALSE, "failed with last error ", ::GetLastError());
			hEvent = nullptr;

			aliases.clear();
			directories.clear();

			debug_assert(active == false);
		}

		system::system(directory && root)
		{
			if (!debug_assert(hThread == nullptr))
				return;

			if (!debug_assert(active == false))
				return;

			if (!debug_assert(hEvent == nullptr))
				return;

			const auto root_asset = make_asset(root.filepath_);

			if (!debug_verify(directories.emplace<Directory>(root_asset, std::move(root.filepath_))))
				return;

			if (!debug_verify(aliases.emplace<Alias>(engine::file::working_directory, root_asset)))
			{
				directories.clear();
				return;
			}

			hEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!debug_verify(hEvent != nullptr, "CreateEventW failed with last error ", ::GetLastError()))
			{
				aliases.clear();
				directories.clear();
				return;
			}

			active = true;

			hThread = ::CreateThread(nullptr, 0, file_watch, nullptr, 0, nullptr);
			if (!debug_verify(hThread != nullptr, "CreateThread failed with last error ", ::GetLastError()))
			{
				active = false;
				debug_verify(::CloseHandle(hEvent) != FALSE, "failed with last error ", ::GetLastError());
				hEvent = nullptr;
				aliases.clear();
				directories.clear();
				return;
			}
		}

		void register_directory(
			system & /*system*/,
			engine::Asset name,
			utility::heap_string_utf8 && path,
			engine::Asset parent)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RegisterDirectory>(name, parent, std::move(path));
			}
		}

		void register_temporary_directory(
			system & /*system*/,
			engine::Asset name)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RegisterTemporaryDirectory>(name);
			}
		}

		void unregister_directory(
			system & /*system*/,
			engine::Asset name)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<UnregisterDirectory>(name);
			}
		}

		void read(
			system & /*system*/,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			read_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Read>(directory, std::move(filepath), callback, std::move(data), mode);
			}
		}

		void remove_watch(
			system & /*system*/,
			engine::Asset directory)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RemoveDirectoryWatch>(directory);
			}
		}

		void remove_watch(
			system & /*system*/,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RemoveFileWatch>(directory, std::move(filepath));
			}
		}

		void scan(
			system & /*system*/,
			engine::Asset directory,
			scan_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Scan>(directory, callback, std::move(data), mode);
			}
		}

		void write(
			system & /*system*/,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Write>(directory, std::move(filepath), callback, std::move(data), mode);
			}
		}
	}
}

#endif
