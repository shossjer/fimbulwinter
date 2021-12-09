#include "config.h"

#if FILE_SYSTEM_USE_KERNEL32

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"
#include "core/file/paths.hpp"
#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/system.hpp"
#include "engine/file/watch/watch.hpp"
#include "engine/HashTable.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"
#include "utility/optional.hpp"
#include "utility/shared_ptr.hpp"

#include <Windows.h>

static_hashes("_working directory_");

namespace
{
	engine::Hash make_asset(utility::string_units_utfw filepath)
	{
		// todo maybe add wchar_t support for Asset?
		return engine::Hash(reinterpret_cast<const char *>(filepath.data()), filepath.size() * sizeof(wchar_t));
	}
}

namespace
{
	struct Directory
	{
		utility::heap_string_utfw dirpath;

		ext::ssize share_count;

		explicit Directory(utility::heap_string_utfw && dirpath)
			: dirpath(std::move(dirpath))
			, share_count(0)
		{}
	};

	struct TemporaryDirectory
	{
		utility::heap_string_utfw dirpath;

		explicit TemporaryDirectory(utility::heap_string_utfw && dirpath)
			: dirpath(std::move(dirpath))
		{}
	};

	struct Alias
	{
		engine::Token directory;
	};
}

namespace engine
{
	namespace file
	{
		struct system_impl
		{
			engine::task::scheduler * taskscheduler;

			engine::file::watch_impl watch_impl;

			engine::Hash strand;

			core::container::Collection
			<
				engine::Token,
				utility::heap_storage_traits,
				utility::heap_storage<Directory>,
				utility::heap_storage<TemporaryDirectory>
			>
			directories;

			core::container::Collection
			<
				engine::Token,
				utility::heap_storage_traits,
				utility::heap_storage<Alias>
			>
			aliases;

			HANDLE hThread;
			HANDLE hTerminateEvent;

			const utility::heap_string_utfw & get_dirpath(decltype(directories)::const_iterator it)
			{
				return directories.call(it, [](const auto & x) -> const utility::heap_string_utfw & { return x.dirpath; });
			}
		};
	}
}

namespace
{
	utility::spinlock singelton_lock;
	utility::optional<engine::file::system_impl> singelton;

	engine::file::system_impl * create_impl()
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		if (singelton)
			return nullptr;

		singelton.emplace();

		return &singelton.value();
	}

	void destroy_impl(engine::file::system_impl & /*impl*/)
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		singelton.reset();
	}
}

namespace
{
	void scan_directory(const utility::heap_string_utfw & dirpath, bool recurse, utility::heap_string_utfw & files)
	{
		debug_printline("scanning directory \"", utility::heap_narrow<utility::encoding_utf8>(dirpath), "\"");

		utility::heap_vector<utility::heap_string_utfw> subdirs;
		if (!debug_verify(subdirs.try_emplace_back()))
			return; // error

		utility::heap_string_utfw pattern;
		if (!debug_verify(pattern.try_append(dirpath)))
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
			debug_verify(error == ERROR_NO_MORE_FILES, "FindNextFileW failed with last error ", error);

			debug_verify(::FindClose(hFile) != FALSE, "failed with last error ", ::GetLastError());

			pattern.reduce(subdir.size() + 1); // * is one character
		}

		if (!empty(files))
		{
			files.reduce(1); // trailing ;
		}
	}

	bool read_file(engine::file::system_impl & impl, utility::heap_string_utfw & filepath, std::uint32_t root, engine::file::read_callback * callback, utility::any & data, FILETIME & last_write_time)
	{
		HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const auto error = ::GetLastError();
			debug_inform(hFile != INVALID_HANDLE_VALUE, "CreateFileW(\"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\") failed with last error ", error);
			return debug_verify((error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND || error == ERROR_SHARING_VIOLATION));
		}

		BY_HANDLE_FILE_INFORMATION by_handle_file_information;
		if (debug_verify(::GetFileInformationByHandle(hFile, &by_handle_file_information) != 0, "GetFileInformationByHandle failed with last error ", ::GetLastError()))
		{
			if (!debug_inform((by_handle_file_information.ftLastWriteTime.dwHighDateTime != last_write_time.dwHighDateTime ||
			                   by_handle_file_information.ftLastWriteTime.dwLowDateTime != last_write_time.dwLowDateTime), "FileReadWork did not detect any change"))
			{
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return true;
			}
			last_write_time = by_handle_file_information.ftLastWriteTime;
		}

		utility::heap_string_utf8 relpath;
		if (!debug_verify(utility::try_narrow(utility::string_points_utfw(filepath.data() + root, filepath.data() + filepath.size()), relpath)))
			return false; // error

		core::file::backslash_to_slash(relpath);

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

		engine::file::system filesystem(impl);
		callback(filesystem, std::move(stream), data);
		filesystem.detach();

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

		return true;
	}

	bool write_file(engine::file::system_impl & impl, utility::heap_string_utfw & filepath, std::uint32_t root, engine::file::write_callback * callback, utility::any & data, bool append, bool overwrite)
	{
		DWORD dwCreationDisposition = CREATE_NEW;
		DWORD dwDesiredAccess = GENERIC_WRITE;
		if (append)
		{
			dwCreationDisposition = OPEN_ALWAYS;
			dwDesiredAccess = FILE_APPEND_DATA;
		}
		else if (overwrite)
		{
			dwCreationDisposition = CREATE_ALWAYS;
		}
		HANDLE hFile = ::CreateFileW(filepath.data(), dwDesiredAccess, 0, nullptr, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			const auto error = ::GetLastError();
			debug_inform(hFile != INVALID_HANDLE_VALUE, "CreateFileW(\"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\") failed with last error ", error);
			return debug_verify(error == ERROR_FILE_EXISTS);
		}

		utility::heap_string_utf8 relpath;
		if (!debug_verify(utility::try_narrow(utility::string_points_utfw(filepath.data() + root, filepath.data() + filepath.size()), relpath)))
			return false; // error

		core::file::backslash_to_slash(relpath);

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
			std::move(relpath));

		engine::file::system filesystem(impl);
		callback(filesystem, std::move(stream), std::move(data));
		filesystem.detach();

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

		return true;
	}

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

	utility::const_string_iterator<utility::boundary_unit_utf8> check_filepath(utility::const_string_iterator<utility::boundary_unit_utf8> begin, utility::const_string_iterator<utility::boundary_unit_utf8> end)
	{
		// todo disallow windows absolute paths

		// todo report \\ as error

		while (true)
		{
			const auto slash = find(begin, end, '/');
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

	bool add_file(utility::heap_string_utfw & files, utility::string_units_utfw subdir, utility::string_units_utfw filename)
	{
		auto begin = files.begin();
		const auto end = files.end();
		if (begin != end)
		{
			while (true)
			{
				const auto split = find(begin, end, L';');
				if (utility::starts_with(begin, end, subdir))
				{
					const auto dir_skip = begin + subdir.size();
					if (utility::string_units_utfw(dir_skip, split) == filename)
						return false; // already added
				}

				if (split == end)
					break;

				begin = split + 1; // skip ;
			}

			if (!debug_verify(files.try_push_back(L';')))
				return false;
		}

		if (!(debug_verify(files.try_append(subdir)) &&
		      debug_verify(files.try_append(filename))))
		{
			files.erase(begin == end ? files.begin() : rfind(files, L';'), files.end());
			return false;
		}

		return true;
	}

	bool remove_file(utility::heap_string_utfw & files, utility::string_units_utfw subdir, utility::string_units_utfw filename)
	{
		auto begin = files.begin();
		const auto end = files.end();

		auto prev_split = begin;
		while (true)
		{
			const auto split = find(begin, end, L';');
			if (split == end)
				break;

			const auto next_begin = split + 1; // skip ;

			if (starts_with(begin, split, subdir) && utility::string_units_utfw(begin + subdir.size(), split) == filename)
			{
				files.erase(begin, next_begin);

				return true;
			}

			prev_split = split;
			begin = next_begin;
		}

		if (starts_with(begin, end, subdir) && utility::string_units_utfw(begin + subdir.size(), end) == filename)
		{
			files.erase(prev_split, end);

			return true;
		}

		return false; // already removed
	}

	void remove_duplicates(utility::heap_string_utfw & files, utility::string_units_utfw duplicates)
	{
		auto duplicates_begin = duplicates.begin();
		const auto duplicates_end = duplicates.end();

		if (duplicates_begin != duplicates_end)
		{
			while (true)
			{
				const auto duplicates_split = find(duplicates_begin, duplicates_end, L';');
				if (!debug_assert(duplicates_split != duplicates_begin, "unexpected file without name"))
					return; // error

				remove_file(files, L"", utility::string_units_utfw(duplicates_begin, duplicates_split));

				if (duplicates_split == duplicates_end)
					break;

				duplicates_begin = duplicates_split + 1;
			}
		}
	}
}

namespace engine
{
	namespace file
	{
		void post_work(FileMissingWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileMissingWork>()))
				{
					FileMissingWork && work = utility::any_cast<FileMissingWork &&>(std::move(data));
					engine::file::ReadData & read_data = *work.ptr;

					if (!debug_inform((DWORD(-1) != read_data.last_write_time.dwHighDateTime || DWORD(-1) != read_data.last_write_time.dwLowDateTime), "FileReadWork did not detect any change"))
						return; // todo no change

					read_data.last_write_time.dwHighDateTime = DWORD(-1);
					read_data.last_write_time.dwLowDateTime = DWORD(-1);

					utility::heap_string_utf8 relpath;
					if (!debug_verify(utility::try_narrow(utility::string_points_utfw(read_data.filepath.data() + read_data.root, read_data.filepath.data() + read_data.filepath.size()), relpath)))
						return; // error

					core::file::backslash_to_slash(relpath);

					core::ReadStream stream(std::move(relpath));

					engine::file::system filesystem(read_data.impl);
					read_data.callback(filesystem, std::move(stream), read_data.data);
					filesystem.detach();
				}
			},
				utility::any(std::move(data)));
		}

		void post_work(FileReadWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileReadWork>()))
				{
					FileReadWork && work = utility::any_cast<FileReadWork &&>(std::move(data));
					engine::file::ReadData & read_data = *work.ptr;

					if (read_file(read_data.impl, read_data.filepath, read_data.root, read_data.callback, read_data.data, read_data.last_write_time))
					{
					}
					else
					{
						if (!debug_inform((DWORD(-1) != read_data.last_write_time.dwHighDateTime || DWORD(-1) != read_data.last_write_time.dwLowDateTime), "FileReadWork did not detect any change"))
							return; // todo no change

						read_data.last_write_time.dwHighDateTime = DWORD(-1);
						read_data.last_write_time.dwLowDateTime = DWORD(-1);

						utility::heap_string_utf8 relpath;
						if (!debug_verify(utility::try_narrow(utility::string_points_utfw(read_data.filepath.data() + read_data.root, read_data.filepath.data() + read_data.filepath.size()), relpath)))
							return; // error

						core::file::backslash_to_slash(relpath);

						core::ReadStream stream(std::move(relpath));

						engine::file::system filesystem(read_data.impl);
						read_data.callback(filesystem, std::move(stream), read_data.data);
						filesystem.detach();
					}
				}
			},
				utility::any(std::move(data)));
		}

		void post_work(ScanChangeWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<ScanChangeWork>()))
				{
					ScanChangeWork && work = utility::any_cast<ScanChangeWork &&>(std::move(data));
					engine::file::ScanData & scan_data = *work.ptr;

					utility::heap_string_utfw old_files = scan_data.files;
					// todo garbage callback if copy fails

					utility::string_units_utfw subdir(work.filepath.begin() + scan_data.dirpath.size(), work.filepath.end());

					for (const auto & file : work.files)
					{
						switch (front(file))
						{
						case '+':
							add_file(scan_data.files, subdir, utility::string_units_utfw(file.begin() + 1, file.end()));
							break;
						case '-':
							remove_file(scan_data.files, subdir, utility::string_units_utfw(file.begin() + 1, file.end()));
							break;
						default:
							debug_unreachable("unknown file change");
						}
					}

					remove_duplicates(old_files, scan_data.files);

					utility::heap_string_utf8 existing_files_utf8;
					if (!debug_verify(utility::try_narrow(scan_data.files, existing_files_utf8)))
						return; // error

					utility::heap_string_utf8 removed_files_utf8;
					if (!debug_verify(utility::try_narrow(old_files, removed_files_utf8)))
						return; // error

					core::file::backslash_to_slash(existing_files_utf8);
					core::file::backslash_to_slash(removed_files_utf8);

					engine::file::system filesystem(scan_data.impl);
					scan_data.callback(filesystem, scan_data.directory, std::move(existing_files_utf8), std::move(removed_files_utf8), scan_data.data);
					filesystem.detach();
				}
			},
				utility::any(std::move(data)));
		}

		void post_work(ScanOnceWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<ScanOnceWork>()))
				{
					ScanOnceWork && work = utility::any_cast<ScanOnceWork &&>(std::move(data));
					engine::file::ScanData & scan_data = *work.ptr;

					utility::heap_string_utfw old_files = std::move(scan_data.files);
					scan_directory(scan_data.dirpath, false, scan_data.files);

					remove_duplicates(old_files, scan_data.files);

					utility::heap_string_utf8 existing_files_utf8;
					if (!debug_verify(utility::try_narrow(scan_data.files, existing_files_utf8)))
						return; // error

					utility::heap_string_utf8 removed_files_utf8;
					if (!debug_verify(utility::try_narrow(old_files, removed_files_utf8)))
						return; // error

					core::file::backslash_to_slash(existing_files_utf8);
					core::file::backslash_to_slash(removed_files_utf8);

					engine::file::system filesystem(scan_data.impl);
					scan_data.callback(filesystem, scan_data.directory, std::move(existing_files_utf8), std::move(removed_files_utf8), scan_data.data);
					filesystem.detach();
				}
			},
				utility::any(std::move(data)));
		}

		void post_work(ScanRecursiveWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<ScanRecursiveWork>()))
				{
					ScanRecursiveWork && work = utility::any_cast<ScanRecursiveWork &&>(std::move(data));
					engine::file::ScanData & scan_data = *work.ptr;

					utility::heap_string_utfw old_files = std::move(scan_data.files);
					scan_directory(scan_data.dirpath, true, scan_data.files);

					remove_duplicates(old_files, scan_data.files);

					utility::heap_string_utf8 existing_files_utf8;
					if (!debug_verify(utility::try_narrow(scan_data.files, existing_files_utf8)))
						return; // error

					utility::heap_string_utf8 removed_files_utf8;
					if (!debug_verify(utility::try_narrow(old_files, removed_files_utf8)))
						return; // error

					core::file::backslash_to_slash(existing_files_utf8);
					core::file::backslash_to_slash(removed_files_utf8);

					engine::file::system filesystem(scan_data.impl);
					scan_data.callback(filesystem, scan_data.directory, std::move(existing_files_utf8), std::move(removed_files_utf8), scan_data.data);
					filesystem.detach();
				}
			},
				utility::any(std::move(data)));
		}

		void post_work(FileWriteWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Hash strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Hash /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileWriteWork>()))
				{
					FileWriteWork && work = utility::any_cast<FileWriteWork &&>(std::move(data));
					engine::file::WriteData & write_data = *work.ptr;

					if (write_file(write_data.impl, write_data.filepath, write_data.root, write_data.callback, write_data.data, write_data.append, write_data.overwrite))
					{
					}
					else
					{
						utility::heap_string_utf8 relpath;
						if (!debug_verify(utility::try_narrow(utility::string_points_utfw(write_data.filepath.data() + write_data.root, write_data.filepath.data() + write_data.filepath.size()), relpath)))
							return; // error

						core::file::backslash_to_slash(relpath);

						core::WriteStream stream(std::move(relpath));

						engine::file::system filesystem(write_data.impl);
						write_data.callback(filesystem, std::move(stream), std::move(write_data.data));
						filesystem.detach();
					}
				}
			},
				utility::any(std::move(data)));
		}
	}
}

namespace
{
	struct ProcessRegisterDirectory
	{
		engine::file::system_impl & impl;
		engine::Hash alias;
		utility::heap_string_utf8 filepath;
		engine::Hash parent;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessRegisterDirectory> data(reinterpret_cast<ProcessRegisterDirectory *>(Parameter));
			ProcessRegisterDirectory & x = *data;

			if (!debug_verify(find(x.impl.aliases, x.alias) == x.impl.aliases.end()))
				return; // error

			const auto parent_alias_it = find(x.impl.aliases, x.parent);
			if (!debug_verify(parent_alias_it != x.impl.aliases.end()))
				return; // error

			if (!debug_verify(validate_filepath(x.filepath)))
				return; // error

			const auto parent_directory_it = find(x.impl.directories, x.impl.aliases.get<Alias>(parent_alias_it)->directory);
			if (!debug_assert(parent_directory_it != x.impl.directories.end()))
				return;

			const auto & dirpath = x.impl.get_dirpath(parent_directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(dirpath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			core::file::slash_to_backslash(filepath.begin() + dirpath.size(), filepath.end());

			if (back(filepath) != L'\\')
			{
				if (!debug_verify(filepath.try_push_back(L'\\')))
					return; // error
			}

			const auto directory_asset = make_asset(filepath);
			const auto directory_it = find(x.impl.directories, directory_asset);
			if (directory_it != x.impl.directories.end())
			{
				const auto directory_ptr = x.impl.directories.get<Directory>(directory_it);
				if (!debug_assert(directory_ptr))
					return;

				directory_ptr->share_count++;
			}
			else
			{
				if (!debug_verify(x.impl.directories.emplace<Directory>(directory_asset, std::move(filepath))))
					return; // error
			}

			if (!debug_verify(x.impl.aliases.emplace<Alias>(x.alias, directory_asset)))
				return; // error
		}
	};

	struct ProcessRegisterTemporaryDirectory
	{
		engine::file::system_impl & impl;
		engine::Hash alias;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessRegisterTemporaryDirectory> data(reinterpret_cast<ProcessRegisterTemporaryDirectory *>(Parameter));
			ProcessRegisterTemporaryDirectory & x = *data;

			// todo generate directory name some other way
			utility::static_string_utfw<(MAX_PATH + 1 + 1)> temppath(MAX_PATH + 1); // GetTempPathW returns at most (MAX_PATH + 1) (not counting null terminator)
			if (!debug_verify(::GetTempPathW(MAX_PATH + 1 + 1, temppath.data()) != 0, "failed with last error ", ::GetLastError()))
				return;

			utility::static_string_utfw<MAX_PATH> tempfilepath(MAX_PATH - 1); // GetTempFileNameW returns at most MAX_PATH (including null terminator)
			if (!debug_verify(::GetTempFileNameW(temppath.data(), L"unn", 0, tempfilepath.data()) != 0, "failed with last error ", ::GetLastError()))
				return;

			if (!debug_verify(::DeleteFileW(tempfilepath.data()) != FALSE, "failed with last error ", ::GetLastError()))
				return;

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(static_cast<const wchar_t *>(tempfilepath.data()))))
				return; // error

			if (!debug_verify(::CreateDirectoryW(filepath.data(), nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				return;

			debug_printline("created temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

			if (!debug_verify(filepath.try_push_back(L'\\')))
			{
				purge_temporary_directory(filepath);
				return; // error
			}

			const auto directory_asset = make_asset(filepath);
			if (!debug_verify(x.impl.directories.emplace<TemporaryDirectory>(directory_asset, std::move(filepath))))
			{
				purge_temporary_directory(filepath);
				return; // error
			}

			if (!debug_verify(x.impl.aliases.emplace<Alias>(x.alias, directory_asset)))
				return; // error
		}
	};

	struct ProcessUnregisterDirectory
	{
		engine::file::system_impl & impl;
		engine::Hash alias;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessUnregisterDirectory> data(reinterpret_cast<ProcessUnregisterDirectory *>(Parameter));
			ProcessUnregisterDirectory & x = *data;

			const auto alias_it = find(x.impl.aliases, x.alias);
			if (!debug_verify(alias_it != x.impl.aliases.end(), "alias does not exist"))
				return; // error

			const auto alias_ptr = x.impl.aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(x.impl.directories, alias_ptr->directory);
			if (!debug_assert(directory_it != x.impl.directories.end()))
				return;

			struct
			{
				bool operator () (Directory & x) { x.share_count--; return x.share_count < 0; }
				bool operator () (TemporaryDirectory & x)
				{
					purge_temporary_directory(x.dirpath);
					return true;
				}
			}
			detach_alias;

			const auto directory_is_unused = x.impl.directories.call(directory_it, detach_alias);
			if (directory_is_unused)
			{
				x.impl.directories.erase(directory_it);
			}

			x.impl.aliases.erase(alias_it);
		}
	};

	struct ProcessRead
	{
		engine::file::system_impl & impl;
		engine::Token id;
		engine::Hash directory;
		utility::heap_string_utf8 filepath;
		engine::Hash strand;
		engine::file::read_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessRead> data(reinterpret_cast<ProcessRead *>(Parameter));
			ProcessRead & x = *data;

			const auto alias_it = find(x.impl.aliases, x.directory);
			if (!debug_verify(alias_it != x.impl.aliases.end()))
				return; // error

			const auto alias_ptr = x.impl.aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(x.impl.directories, alias_ptr->directory);
			if (!debug_assert(directory_it != x.impl.directories.end()))
				return;

			const auto & dirpath = x.impl.get_dirpath(directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(dirpath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			core::file::slash_to_backslash(filepath.begin() + dirpath.size(), filepath.end());

			ext::heap_shared_ptr<engine::file::ReadData> ptr(utility::in_place, x.impl, std::move(filepath), static_cast<std::uint32_t>(dirpath.size()), x.strand, x.callback, std::move(x.data), FILETIME{});
			if (!debug_verify(ptr))
				return; // error

			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				engine::file::add_file_watch(x.impl.watch_impl, x.id, ptr, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING));
			}

			engine::file::post_work(engine::file::FileReadWork{std::move(ptr)});
		}
	};

	struct ProcessRemoveWatch
	{
		engine::file::system_impl & impl;
		engine::Token id;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessRemoveWatch> data(reinterpret_cast<ProcessRemoveWatch *>(Parameter));
			ProcessRemoveWatch & x = *data;

			engine::file::remove_watch(x.impl.watch_impl, x.id);
		}
	};

	struct ProcessScan
	{
		engine::file::system_impl & impl;
		engine::Token id;
		engine::Hash directory;
		engine::Hash strand;
		engine::file::scan_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessScan> data(reinterpret_cast<ProcessScan *>(Parameter));
			ProcessScan & x = *data;

			const auto alias_it = find(x.impl.aliases, x.directory);
			if (!debug_verify(alias_it != x.impl.aliases.end()))
				return; // error

			const auto alias_ptr = x.impl.aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(x.impl.directories, alias_ptr->directory);
			if (!debug_assert(directory_it != x.impl.directories.end()))
				return;

			const auto & dirpath = x.impl.get_dirpath(directory_it);

			ext::heap_shared_ptr<engine::file::ScanData> ptr(utility::in_place, x.impl, dirpath, x.directory, x.strand, x.callback, std::move(x.data), utility::heap_string_utfw());
			if (!debug_verify(ptr))
				return; // error

			if (x.mode & engine::file::flags::ADD_WATCH)
			{
				engine::file::add_scan_watch(x.impl.watch_impl, x.id, ptr, static_cast<bool>(x.mode & engine::file::flags::RECURSE_DIRECTORIES));
			}

			if (x.mode & engine::file::flags::RECURSE_DIRECTORIES)
			{
				engine::file::post_work(engine::file::ScanRecursiveWork{std::move(ptr)});
			}
			else
			{
				engine::file::post_work(engine::file::ScanOnceWork{std::move(ptr)});
			}
		}
	};

	struct ProcessWrite
	{
		engine::file::system_impl & impl;
		engine::Hash directory;
		utility::heap_string_utf8 filepath;
		engine::Hash strand;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<ProcessWrite> data(reinterpret_cast<ProcessWrite *>(Parameter));
			ProcessWrite & x = *data;

			const auto alias_it = find(x.impl.aliases, x.directory);
			if (!debug_verify(alias_it != x.impl.aliases.end()))
				return; // error

			const auto alias_ptr = x.impl.aliases.get<Alias>(alias_it);
			if (!debug_assert(alias_ptr))
				return;

			const auto directory_it = find(x.impl.directories, alias_ptr->directory);
			if (!debug_assert(directory_it != x.impl.directories.end()))
				return;

			const auto & dirpath = x.impl.get_dirpath(directory_it);

			utility::heap_string_utfw filepath;
			if (!debug_verify(filepath.try_append(dirpath)))
				return; // error

			if (!debug_verify(utility::try_widen_append(x.filepath, filepath)))
				return; // error

			core::file::slash_to_backslash(filepath.begin() + dirpath.size(), filepath.end());

			ext::heap_shared_ptr<engine::file::WriteData> ptr(utility::in_place, x.impl, filepath, static_cast<std::uint32_t>(dirpath.size()), x.strand, x.callback, std::move(x.data), static_cast<bool>(x.mode & engine::file::flags::APPEND_EXISTING), static_cast<bool>(x.mode & engine::file::flags::OVERWRITE_EXISTING));
			if (!debug_verify(ptr))
				return; // error

			if (x.mode & engine::file::flags::CREATE_DIRECTORIES)
			{
				auto begin = filepath.begin() + dirpath.size();
				const auto end = filepath.end();

				while (true)
				{
					const auto slash = find(begin, end, L'\\');
					if (slash == end)
						break;

					*slash = L'\0';

					if (CreateDirectoryW(filepath.data(), nullptr) == 0)
					{
						if (!debug_verify(::GetLastError() == ERROR_ALREADY_EXISTS))
							return; // error
					}

					*slash = L'\\';

					begin = slash + 1;
				}
			}

			engine::file::post_work(engine::file::FileWriteWork{std::move(ptr)});
		}
	};

	DWORD WINAPI file_watch(LPVOID arg)
	{
		engine::file::system_impl & impl = *static_cast<engine::file::system_impl *>(arg);

		DWORD ret;

		while (true)
		{
			ret = ::WaitForSingleObjectEx(impl.hTerminateEvent, INFINITE, TRUE);
			if (ret != WAIT_IO_COMPLETION)
				break;
		}

		debug_verify(ret != WAIT_FAILED, "WaitForSingleObjectEx failed with last error ", ::GetLastError());

		// todo ensure all watches are stopped (and deleted correctly)

		return 0;
	}

	template <typename T, typename ...Ps>
	bool try_queue_apc(HANDLE hThread, Ps && ...ps)
	{
		if (!debug_assert(hThread != nullptr))
			return false;

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

			const auto len = ::GetCurrentDirectoryW(0, nullptr); // including null
			if (!debug_verify(len != 0, ::GetLastError()))
				return dir;

			if (!debug_verify(dir.filepath_.try_resize(len)))
				return dir;

			if (!debug_verify(::GetCurrentDirectoryW(len, dir.filepath_.data()) != 0, ::GetLastError()))
				return dir;

			dir.filepath_.data()[len - 1] = L'\\';

			return dir;
		}

		void system::destruct(system_impl & impl)
		{
			::SetEvent(impl.hTerminateEvent);

			debug_verify(::WaitForSingleObject(impl.hThread, INFINITE) != WAIT_FAILED, "failed with last error ", ::GetLastError());

			debug_verify(::CloseHandle(impl.hThread) != FALSE, "failed with last error ", ::GetLastError());
			debug_verify(::CloseHandle(impl.hTerminateEvent) != FALSE, "failed with last error ", ::GetLastError());

			destroy_impl(impl);
		}

		system_impl * system::construct(engine::task::scheduler & taskscheduler, directory && root)
		{
			system_impl * const impl = create_impl();
			if (debug_verify(impl))
			{
				impl->taskscheduler = &taskscheduler;

				const auto root_asset = make_asset(root.filepath_);

				if (!debug_verify(impl->directories.emplace<Directory>(root_asset, std::move(root.filepath_))))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				if (!debug_verify(impl->aliases.emplace<Alias>(engine::file::working_directory, root_asset)))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				impl->hTerminateEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
				if (!debug_verify(impl->hTerminateEvent != nullptr, "CreateEventW failed with last error ", ::GetLastError()))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				impl->hThread = ::CreateThread(nullptr, 0, file_watch, impl, 0, nullptr);
				if (!debug_verify(impl->hThread != nullptr, "CreateThread failed with last error ", ::GetLastError()))
				{
					debug_verify(::CloseHandle(impl->hTerminateEvent) != FALSE, "failed with last error ", ::GetLastError());

					destroy_impl(*impl);
					return nullptr;
				}
			}
			return impl;
		}

		void register_directory(
			system & system,
			engine::Hash name,
			utility::heap_string_utf8 && filepath,
			engine::Hash parent)
		{
			try_queue_apc<ProcessRegisterDirectory>(system->hThread, *system, name, std::move(filepath), parent);
		}

		void register_temporary_directory(
			system & system,
			engine::Hash name)
		{
			try_queue_apc<ProcessRegisterTemporaryDirectory>(system->hThread, *system, name);
		}

		void unregister_directory(
			system & system,
			engine::Hash name)
		{
			try_queue_apc<ProcessUnregisterDirectory>(system->hThread, *system, name);
		}

		void read(
			system & system,
			engine::Token id,
			engine::Hash directory,
			utility::heap_string_utf8 && filepath,
			engine::Hash strand,
			read_callback * callback,
			utility::any && data,
			flags mode)
		{
			try_queue_apc<ProcessRead>(system->hThread, *system, id, directory, std::move(filepath), strand, callback, std::move(data), mode);
		}

		void remove_watch(
			system & system,
			engine::Token id)
		{
			try_queue_apc<ProcessRemoveWatch>(system->hThread, *system, id);
		}

		void scan(
			system & system,
			engine::Token id,
			engine::Hash directory,
			engine::Hash strand,
			scan_callback * callback,
			utility::any && data,
			flags mode)
		{
			try_queue_apc<ProcessScan>(system->hThread, *system, id, directory, strand, callback, std::move(data), mode);
		}

		void write(
			system & system,
			engine::Hash directory,
			utility::heap_string_utf8 && filepath,
			engine::Hash strand,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			try_queue_apc<ProcessWrite>(system->hThread, *system, directory, std::move(filepath), strand, callback, std::move(data), mode);
		}
	}
}

#endif
