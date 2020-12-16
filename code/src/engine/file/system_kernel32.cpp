#include "config.h"

#if FILE_USE_KERNEL32

#include "core/container/Collection.hpp"
#include "core/file/paths.hpp"
#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/functional/utility.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/string.hpp"

#include "engine/file/system.hpp"
#include "engine/task/scheduler.hpp"

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
	engine::task::scheduler * module_taskscheduler = nullptr;

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

	struct FileReadData
	{
		Path path;

		engine::Asset strand; // todo not needed within the workcall
		engine::file::read_callback * callback;
		utility::any data;

		FILETIME last_write_time;
	};

	struct FileScanCallData
	{
		engine::Asset directory;
		engine::Asset strand; // todo not needed within the workcall
		engine::file::scan_callback * callback;
		utility::any data;
	};

	struct FileWriteData
	{
		Path path;

		engine::Asset strand; // todo not needed within the workcall
		engine::file::write_callback * callback;
		utility::any data;
	};

	using FileReadPointer = ext::heap_shared_ptr<FileReadData>;
	using FileScanCallPtr = ext::heap_shared_ptr<FileScanCallData>;
	using FileWritePointer = ext::heap_shared_ptr<FileWriteData>; // todo unique

	struct FileScanData
	{
		FileScanCallPtr call_ptr;
		utility::heap_string_utfw files;
	};

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	// owned by the completion routine
	struct WatchData
	{
		OVERLAPPED overlapped;

		Path path;
		engine::file::flags flags;

		utility::heap_vector<utility::heap_string_utfw, FileReadPointer> reads;
		utility::heap_vector<engine::Asset, FileScanCallPtr> scans;

		utility::heap_string_utfw files;

		std::aligned_storage_t<1024, alignof(DWORD)> buffer; // arbitrary

		explicit WatchData(HANDLE hDirectory, Path && path, engine::file::flags flags)
			: path(std::move(path))
			, flags(flags)
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

	utility::heap_vector<std::pair<engine::Asset, engine::file::flags>, WatchData *> watches;

	WatchData * start_watch(engine::Asset id, Path && path, engine::file::flags flags)
	{
		if (!debug_assert(!ext::contains_if(watches, fun::first == std::make_pair(id, flags))))
			return nullptr;

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

		WatchData * const data = new WatchData(hDirectory, std::move(path), flags);
		if (!debug_assert(data))
		{
			debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			return nullptr;
		}

		if (!debug_verify(watches.try_emplace_back(std::make_pair(id, flags), data)))
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

	bool add_file(utility::heap_string_utfw & files, const Path & path, utility::string_units_utfw filename)
	{
		auto begin = files.begin();
		const auto end = files.end();
		const auto undo_size = files.size();
		if (begin != end)
		{
			while (true)
			{
				const auto dir_skip = begin + (path.filepath.size() - path.root);
				const auto split = find(dir_skip, end, L';');
				if (utility::string_units_utfw(dir_skip, split) == filename)
					return false; // already added

				if (split == end)
					break;

				begin = split + 1; // skip ;
			}

			if (!debug_verify(files.try_push_back(L';')))
				return false;
		}
		if (!debug_verify(files.try_append(path.filepath, path.root)))
		{
			files.reduce(files.size() - undo_size);
			return false;
		}
		if (!debug_verify(files.try_append(filename)))
		{
			files.reduce(files.size() - undo_size);
			return false;
		}

		return true;
	}

	bool remove_file(utility::heap_string_utfw & files, const Path & path, utility::string_units_utfw filename)
	{
		auto begin = files.begin();
		auto prev_split = files.begin();
		const auto end = files.end();
		if (begin != end)
		{
			while (true)
			{
				const auto dir_skip = begin + (path.filepath.size() - path.root);
				const auto split = find(dir_skip, end, L';');
				if (utility::string_units_utfw(dir_skip, split) == filename)
				{
					files.erase(prev_split, split);

					return true;
				}

				if (split == end)
					break;

				prev_split = split;
				begin = split + 1; // skip ;
			}
		}
		return false; // already removed
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

						if (!debug_verify(dir.try_push_back(L'\\')))
							return; // error
					}
				}
				else
				{
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

	bool read_file(const Path & path, engine::file::read_callback * callback, utility::any & data, FILETIME & last_write_time)
	{
		HANDLE hFile = ::CreateFileW(path.filepath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(path.filepath), "\" failed with last error ", ::GetLastError()))
			return false;

		BY_HANDLE_FILE_INFORMATION by_handle_file_information;
		if (debug_verify(::GetFileInformationByHandle(hFile, &by_handle_file_information) != 0, "GetFileInformationByHandle failed with last error ", ::GetLastError()))
		{
			if (by_handle_file_information.ftLastWriteTime.dwHighDateTime == last_write_time.dwHighDateTime &&
				by_handle_file_information.ftLastWriteTime.dwLowDateTime == last_write_time.dwLowDateTime)
			{
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return true;
			}
			last_write_time = by_handle_file_information.ftLastWriteTime;
		}

		utility::heap_string_utf8 relpath;
		if (!debug_verify(utility::try_narrow(utility::string_points_utfw(path.filepath.data() + path.root, path.filepath.data() + path.filepath.size()), relpath)))
			return false; // error

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
			std::move(relpath));

		callback(std::move(stream), data);

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

		return true;
	}

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
			scan_directory(watch->path, static_cast<bool>(watch->flags & engine::file::flags::RECURSE_DIRECTORIES), watch->files);

			for (auto && scan : watch->scans)
			{
				engine::task::post_work(
					*module_taskscheduler,
					scan.second->strand,
					[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<FileScanData>()))
					{
						FileScanData scan_data = utility::any_cast<FileScanData &&>(std::move(data));
						FileScanCallData & call_data = *scan_data.call_ptr;

						utility::heap_string_utf8 files_utf8;
						if (!debug_verify(utility::try_narrow(scan_data.files, files_utf8)))
							return; // error

						core::file::backslash_to_slash(files_utf8);

						call_data.callback(call_data.directory, std::move(files_utf8), call_data.data);
					}
				},
					FileScanData{scan.second, watch->files});
			}

			if (watch->read())
				return;
		}
		else
		{
			bool file_change = false;

			for (std::size_t offset = 0;;)
			{
				debug_assert(offset % alignof(DWORD) == 0);

				const FILE_NOTIFY_INFORMATION * const info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(reinterpret_cast<const char *>(&watch->buffer) + offset);
				debug_assert(info->FileNameLength % sizeof(wchar_t) == 0);

				const auto filename = utility::string_units_utfw(info->FileName, info->FileName + info->FileNameLength / sizeof(wchar_t));

				switch (info->Action)
				{
				case FILE_ACTION_ADDED:
					debug_printline("FILE_ACTION_ADDED: ", utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filename.data(), filename.data() + filename.size())));
					if (add_file(watch->files, watch->path, filename))
					{
						file_change = true;
					}
					break;
				case FILE_ACTION_REMOVED:
					debug_printline("FILE_ACTION_REMOVED: ", utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filename.data(), filename.data() + filename.size())));
					if (remove_file(watch->files, watch->path, filename))
					{
						file_change = true;
					}
					break;
				case FILE_ACTION_MODIFIED:
					debug_printline("FILE_ACTION_MODIFIED: ", utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filename.data(), filename.data() + filename.size())));
					for (auto && read : watch->reads)
					{
						if (read.first == filename)
						{
							engine::task::post_work(
								*module_taskscheduler,
								read.second->strand,
								[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
							{
								if (debug_assert(data.type_id() == utility::type_id<FileReadPointer>()))
								{
									FileReadPointer data_ptr = utility::any_cast<FileReadPointer &&>(std::move(data));
									FileReadData & d = *data_ptr;

									read_file(d.path, d.callback, d.data, d.last_write_time);
								}
							},
								read.second);
						}
					}
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					debug_printline("FILE_ACTION_RENAMED_OLD_NAME: ", utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filename.data(), filename.data() + filename.size())));
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					debug_printline("FILE_ACTION_RENAMED_NEW_NAME: ", utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filename.data(), filename.data() + filename.size())));
					break;
				default:
					debug_unreachable("unknown action ", info->Action);
				}

				if (info->NextEntryOffset == 0)
					break;

				offset += info->NextEntryOffset;
			}

			if (file_change)
			{
				for (auto && scan : watch->scans)
				{
					engine::task::post_work(
						*module_taskscheduler,
						scan.second->strand,
						[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
					{
						if (debug_assert(data.type_id() == utility::type_id<FileScanData>()))
						{
							FileScanData scan_data = utility::any_cast<FileScanData &&>(std::move(data));
							FileScanCallData & call_data = *scan_data.call_ptr;

							utility::heap_string_utf8 files_utf8;
							if (!debug_verify(utility::try_narrow(scan_data.files, files_utf8)))
								return; // error

							core::file::backslash_to_slash(files_utf8);

							call_data.callback(call_data.directory, std::move(files_utf8), call_data.data);
						}
					},
						FileScanData{scan.second, watch->files});
				}
			}

			if (watch->read())
				return;
		}

		debug_printline("deleting watch \"", utility::heap_narrow<utility::encoding_utf8>(watch->path.filepath), "\"");
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

	HANDLE hThread = nullptr;
	HANDLE hEvent = nullptr;

	DWORD WINAPI file_watch(LPVOID /*arg*/)
	{
		DWORD ret;

		while (true)
		{
			ret = ::WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
			if (ret != WAIT_IO_COMPLETION)
				break;
		}

		if (!debug_verify(ret != WAIT_FAILED, "WaitForSingleObjectEx failed with last error ", ::GetLastError()))
			return DWORD(-1);

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
					debug_expression(const auto watch_it = ext::find_if(watches, (fun::first | fun::first) == directory));
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
				debug_expression(const auto watch_it = ext::find_if(watches, (fun::first | fun::first) == alias_ptr->directory));
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
		engine::Asset strand;
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

			FileReadPointer data_ptr(utility::in_place, Path(std::move(filepath), path.root), x.strand, x.callback, std::move(x.data), FILETIME{});
			if (!debug_verify(data_ptr))
				return; // error

			// todo if x.filepath contains slashes the RECURSE_DIRECTORIES flag must be set or else there are inconsistencies with watch
			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				const auto watch_it = ext::find_if(watches, fun::first == std::make_pair(alias_ptr->directory, x.mode));
				if (watch_it != watches.end())
				{
					utility::heap_string_utfw dirpath;
					if (debug_verify(utility::try_widen_append(x.filepath, dirpath)))
					{
						core::file::slash_to_backslash(dirpath);
						debug_verify((*watch_it.second)->reads.try_emplace_back(std::move(dirpath), data_ptr));
					}
				}
				else
				{
					WatchData * const watch_data = start_watch(alias_ptr->directory, Path(path), x.mode);
					utility::heap_string_utfw dirpath;
					if (debug_verify(utility::try_widen_append(x.filepath, dirpath)))
					{
						core::file::slash_to_backslash(dirpath);
						debug_verify(watch_data->reads.try_emplace_back(std::move(dirpath), data_ptr));
					}
				}
			}

			engine::task::post_work(
				*module_taskscheduler,
				x.strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileReadPointer>()))
				{
					FileReadPointer data_ptr = utility::any_cast<FileReadPointer &&>(std::move(data));
					FileReadData & d = *data_ptr;

					debug_assert(d.last_write_time.dwHighDateTime == 0);
					debug_assert(d.last_write_time.dwLowDateTime == 0);

					FILETIME filetime{};
					if (read_file(d.path, d.callback, d.data, filetime))
					{
						d.last_write_time = filetime;
					}
				}
			},
				std::move(data_ptr));
		}
	};

	struct RemoveDirectoryWatch
	{
		engine::Asset directory;
		engine::file::flags mode; // todo a bit weird

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

			const auto watch_it = ext::find_if(watches, fun::first == std::make_pair(alias_ptr->directory, x.mode));
			if (!debug_verify(watch_it != watches.end()))
				return; // error

			const auto scan_it = ext::find_if((*watch_it.second)->scans, fun::first == x.directory);
			if (!debug_verify(scan_it != (*watch_it.second)->scans.end()))
				return; // error

			debug_printline("removing ", scan_it.second->get());
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
		engine::file::flags mode; // todo a bit weird

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

			const auto watch_it = ext::find_if(watches, fun::first == std::make_pair(alias_ptr->directory, x.mode));
			if (!debug_verify(watch_it != watches.end()))
				return; // error

			utility::heap_string_utfw filepath;
			if (!debug_verify(utility::try_widen(x.filepath, filepath)))
				return; // error

			core::file::slash_to_backslash(filepath);
			const auto read_it = ext::find_if((*watch_it.second)->reads, fun::first == filepath);
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
		engine::Asset strand;
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
			const auto watch_it = ext::find_if(watches, fun::first == std::make_pair(alias_ptr->directory, x.mode));
			if (watch_it != watches.end())
			{
				files = (*watch_it.second)->files;
			}
			else
			{
				scan_directory(path, static_cast<bool>(x.mode & engine::file::flags::RECURSE_DIRECTORIES), files);
			}

			FileScanCallPtr call_ptr(utility::in_place, x.directory, x.strand, x.callback, std::move(x.data));
			if (!debug_verify(call_ptr))
				return; // error

			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				if (watch_it != watches.end())
				{
					debug_verify((*watch_it.second)->scans.try_emplace_back(x.directory, call_ptr));
				}
				else
				{
					if (WatchData * const watch_data = start_watch(alias_ptr->directory, Path(path), x.mode))
					{
						debug_verify(watch_data->scans.try_emplace_back(x.directory, call_ptr));
						watch_data->files = files;
					}
				}
			}

			engine::task::post_work(
				*module_taskscheduler,
				x.strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileScanData>()))
				{
					FileScanData scan_data = utility::any_cast<FileScanData &&>(std::move(data));
					FileScanCallData & call_data = *scan_data.call_ptr;

					utility::heap_string_utf8 files_utf8;
					if (!debug_verify(utility::try_narrow(scan_data.files, files_utf8)))
						return; // error

					core::file::backslash_to_slash(files_utf8);

					call_data.callback(call_data.directory, std::move(files_utf8), call_data.data);
				}
			},
				FileScanData{std::move(call_ptr), std::move(files)});
		}
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
		engine::Asset strand;
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

			::SetEvent(hEvent);
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

			module_taskscheduler = nullptr;
		}

		system::system(engine::task::scheduler & taskscheduler, directory && root)
		{
			if (!debug_assert(hThread == nullptr))
				return;

			if (!debug_assert(hEvent == nullptr))
				return;

			module_taskscheduler = &taskscheduler;

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

			hThread = ::CreateThread(nullptr, 0, file_watch, nullptr, 0, nullptr);
			if (!debug_verify(hThread != nullptr, "CreateThread failed with last error ", ::GetLastError()))
			{
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
			engine::Asset strand,
			read_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Read>(directory, std::move(filepath), strand, callback, std::move(data), mode);
			}
		}

		void remove_watch(
			system & /*system*/,
			engine::Asset directory,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RemoveDirectoryWatch>(directory, mode);
			}
		}

		void remove_watch(
			system & /*system*/,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RemoveFileWatch>(directory, std::move(filepath), mode);
			}
		}

		void scan(
			system & /*system*/,
			engine::Asset directory,
			engine::Asset strand,
			scan_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Scan>(directory, strand, callback, std::move(data), mode);
			}
		}

		void write(
			system & /*system*/,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			engine::Asset strand,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Write>(directory, std::move(filepath), strand, callback, std::move(data), mode);
			}
		}
	}
}

#endif
