#include "config.h"

#if FILE_WATCH_USE_KERNEL32

#include "core/container/Collection.hpp"

#include "engine/file/system/works.hpp"
#include "engine/file/watch/watch.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/container/vector.hpp"
#include "utility/functional/utility.hpp"

#include "ful/view.hpp"
#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"
#include "ful/string_search.hpp"

#include <Windows.h>

namespace
{
	engine::Hash make_asset(ful::view_utfw filepath, bool recurse)
	{
		// todo
		return engine::Hash(engine::Hash(reinterpret_cast<const char *>(filepath.data()), filepath.size() * sizeof(wchar_t)) ^ (recurse ? engine::Hash::value_type(-1) : engine::Hash::value_type{}));
	}
}

namespace
{
	struct ReadWatch
	{
		ext::heap_shared_ptr<engine::file::ReadData> ptr;
	};

	struct ScanWatch
	{
		ext::heap_shared_ptr<engine::file::ScanData> ptr;
		bool recurse_directories;
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<ReadWatch>,
		utility::heap_storage<ScanWatch>
	>
	watches;

	// note owned by the completion routine
	struct WatchData
	{
		OVERLAPPED overlapped;

		utility::heap_vector<ful::view_utfw, ext::heap_shared_ptr<engine::file::ReadData>> reads;
		utility::heap_vector<ful::view_utfw, ext::heap_shared_ptr<engine::file::ReadData>> missing_reads;
		utility::heap_vector<ext::heap_shared_ptr<engine::file::ScanData>> scans;

		ful::heap_string_utfw filepath;

		std::aligned_storage_t<1024, alignof(DWORD)> buffer; // arbitrary

		~WatchData()
		{
			debug_verify(::CloseHandle(overlapped.hEvent) != FALSE, "failed with last error ", ::GetLastError());
			debug_expression(overlapped.hEvent = nullptr);
		}

		explicit WatchData(HANDLE hDirectory, ful::heap_string_utfw && filepath)
			: filepath(std::move(filepath))
		{
			// this is fine, see documentation of ReadDirectoryChangesW
			overlapped.hEvent = hDirectory;
		}

		bool read(bool recurse, LPOVERLAPPED_COMPLETION_ROUTINE callback)
		{
			DWORD notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
//#if MODE_DEBUG
//			notify_filter |= FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
//#endif
			return debug_inform(
				::ReadDirectoryChangesW(
					overlapped.hEvent,
					&buffer,
					sizeof buffer,
					recurse ? TRUE : FALSE,
					notify_filter,
					nullptr,
					&overlapped,
					callback) != FALSE,
				"failed with last error ", ::GetLastError());
		}
	};
	static_assert(std::is_standard_layout<WatchData>::value, "WatchData must be pointer-interconvertible with OVERLAPPED!");
	static_assert(offsetof(WatchData, overlapped) == 0, "WatchData must have OVERLAPPED as its first member!");

	struct Alias
	{
		WatchData * watch_data;

		int use_count;
	};

	core::container::Collection
	<
		engine::Hash,
		utility::heap_storage_traits,
		utility::heap_storage<Alias>
	>
	aliases;

	HANDLE start_watch(ful::cstr_utfw dirpath)
	{
		debug_printline("starting watch \"", dirpath, "\"");
		HANDLE hDirectory = ::CreateFileW(
			dirpath.c_str(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);

		if (!debug_verify(hDirectory != INVALID_HANDLE_VALUE, "CreateFileW failed with last error ", ::GetLastError()))
			return nullptr;

		return hDirectory;
	}

	bool stop_watch(WatchData & watch_data)
	{
		debug_printline("stopping watch \"", watch_data.filepath, "\"");
		if (::CancelIoEx(watch_data.overlapped.hEvent, &watch_data.overlapped) == FALSE)
		{
			const auto error = ::GetLastError();
			debug_verify(error == ERROR_NOT_FOUND, "CancelIoEx failed with last error ", error);
			return false;
		}
		return true;
	}

	void Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped, bool recurse, LPOVERLAPPED_COMPLETION_ROUTINE callback)
	{
		WatchData * const watch_data = reinterpret_cast<WatchData *>(lpOverlapped); // note fine since pointer-interconvertible
		if (!debug_assert(lpOverlapped->hEvent))
			return;

		if (!debug_inform(dwErrorCode == 0))
		{
			debug_assert((dwErrorCode == ERROR_ACCESS_DENIED || dwErrorCode == ERROR_OPERATION_ABORTED));
			debug_assert(0 == dwNumberOfBytesTransfered);
		}
		else if (dwNumberOfBytesTransfered == 0)
		{
			for (auto && scan : watch_data->scans)
			{
				engine::file::post_work(engine::file::ScanRecursiveWork{scan});
			}

			if (watch_data->read(recurse, callback))
				return;
		}
		else
		{
			utility::heap_vector<ful::heap_string_utfw> changes;

			for (std::size_t offset = 0;;)
			{
				debug_assert(offset % alignof(DWORD) == 0);

				const FILE_NOTIFY_INFORMATION * const info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(reinterpret_cast<const char *>(&watch_data->buffer) + offset);
				debug_assert(info->FileNameLength % sizeof(wchar_t) == 0);

				const auto filename = ful::view_utfw(info->FileName, info->FileNameLength / sizeof(wchar_t));

				switch (info->Action)
				{
				case FILE_ACTION_ADDED:
					if (debug_verify(changes.try_emplace_back()))
					{
						auto & file = ext::back(changes);
						if (debug_verify(resize(file, filename.size() + 1)))
						{
							file.data()[0] = L'+';
							copy(filename, file.begin() + 1, file.end());
						}
						else
						{
							ext::pop_back(changes);
						}
					}
					break;
				case FILE_ACTION_REMOVED:
					debug_printline("FILE_ACTION_REMOVED: ", filename);
					if (debug_verify(changes.try_emplace_back()))
					{
						auto & file = ext::back(changes);
						if (debug_verify(resize(file, filename.size() + 1)))
						{
							file.data()[0] = L'-';
							copy(filename, file.begin() + 1, file.end());
						}
						else
						{
							ext::pop_back(changes);
						}
					}
					for (auto && read : watch_data->missing_reads)
					{
						if (read.first == filename)
						{
							engine::file::post_work(engine::file::FileMissingWork{read.second});
						}
					}
					break;
				case FILE_ACTION_MODIFIED:
					debug_printline("FILE_ACTION_MODIFIED: ", filename);
					for (auto && read : watch_data->reads)
					{
						if (read.first == filename)
						{
							engine::file::post_work(engine::file::FileReadWork{read.second});
						}
					}
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					debug_printline("FILE_ACTION_RENAMED_OLD_NAME: ", filename);
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					debug_printline("FILE_ACTION_RENAMED_NEW_NAME: ", filename);
					break;
				default:
					debug_unreachable("unknown action ", info->Action);
				}

				if (info->NextEntryOffset == 0)
					break;

				offset += info->NextEntryOffset;
			}

			if (!ext::empty(changes))
			{
				for (auto && scan : watch_data->scans)
				{
					ful::heap_string_utfw filepath;
					if (!debug_verify(ful::copy(watch_data->filepath, filepath)))
						continue; // error ????

					utility::heap_vector<ful::heap_string_utfw> changes_copy;
					if (!changes_copy.try_reserve(changes.size()))
						continue; // error ????

					bool fail = false;
					for (const auto & change : changes)
					{
						if (!changes_copy.try_emplace_back())
						{
							fail = true;
							break;
						}

						if (!debug_verify(ful::copy(change, ext::back(changes_copy))))
						{
							fail = true;
							break;
						}
					}
					if (fail)
						continue; // error ????

					engine::file::post_work(engine::file::ScanChangeWork{scan, std::move(filepath), std::move(changes_copy)});
				}
			}

			if (watch_data->read(recurse, callback))
				return;
		}

		debug_printline("deleting watch \"", watch_data->filepath, "\"");
		delete watch_data;
	}

	void CALLBACK RecursiveCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		Completion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped, true, RecursiveCompletion);
	}

	void CALLBACK NonrecursiveCompletion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		Completion(dwErrorCode, dwNumberOfBytesTransfered, lpOverlapped, false, NonrecursiveCompletion);
	}

	WatchData * get_directory(engine::Hash asset)
	{
		const auto alias_it = find(aliases, asset);
		if (alias_it == aliases.end())
			return nullptr;

		Alias * const alias = aliases.get<Alias>(alias_it);
		if (!debug_assert(alias))
			return nullptr;

		return alias->watch_data;
	}

	WatchData * get_or_create_directory(engine::Hash asset, ful::view_utfw filepath, bool recurse)
	{
		const auto alias_it = find(aliases, asset);
		if (alias_it != aliases.end())
		{
			Alias * const alias = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias))
				return nullptr;

			alias->use_count++;

			return alias->watch_data;
		}
		else
		{
			ful::heap_string_utfw filepath_null;
			if (!debug_verify(ful::copy(filepath, filepath_null)))
				return nullptr; // error

			HANDLE hDirectory = start_watch(ful::cstr_utfw(filepath_null));
			if (!hDirectory)
				return nullptr;

			WatchData * const watch_data = new WatchData(hDirectory, std::move(filepath_null));

			if (!watch_data->read(recurse, recurse ? NonrecursiveCompletion : RecursiveCompletion))
			{
				delete watch_data;
				return nullptr;
			}

			// note from here on, watch_data is owned by the completion routine

			Alias * const alias = aliases.emplace<Alias>(asset, watch_data, 1);
			if (!debug_verify(alias))
			{
				stop_watch(*watch_data);
				return nullptr;
			}

			return watch_data;
		}
	}

	void decrement_alias(engine::Hash asset)
	{
		const auto alias_it = find(aliases, asset);
		if (!debug_assert(alias_it != aliases.end()))
			return;

		Alias * const alias = aliases.get<Alias>(alias_it);
		if (!debug_assert(alias))
			return;

		alias->use_count--;
		if (alias->use_count <= 0)
		{
			WatchData * const watch_data = alias->watch_data;

#if MODE_DEBUG
			if (debug_assert(watch_data))
			{
				debug_inform(ext::empty(watch_data->reads));
				debug_inform(ext::empty(watch_data->missing_reads));
				debug_inform(ext::empty(watch_data->scans));
			}
#endif

			stop_watch(*watch_data);
		}
	}
}

namespace engine
{
	namespace file
	{
		watch_impl::~watch_impl()
		{}

		// todo support multiple watches (for multiple file systems)
		watch_impl::watch_impl()
		{}

		void add_file_watch(engine::file::watch_impl & /*impl*/, engine::Token id, ext::heap_shared_ptr<ReadData> ptr, bool report_missing)
		{
			if (!debug_verify(watches.emplace<ReadWatch>(id, ptr)))
				return;

			const auto directory_split = ful::rfind(ptr->filepath, ful::char16{'\\'});
			if (!debug_assert(directory_split != ptr->filepath.end()))
				return;

			ful::view_utfw filepath(ptr->filepath.begin(), directory_split + 1); // include '\\'
			const auto alias = make_asset(filepath, false);
			if (WatchData * const watch_data = get_or_create_directory(alias, filepath, false))
			{
				ful::view_utfw filename(directory_split + 1, ptr->filepath.end());

				bool using_directory = false;

				using_directory = debug_verify(watch_data->reads.try_emplace_back(filename, ptr)) || using_directory;

				if (report_missing)
				{
					using_directory = debug_verify(watch_data->missing_reads.try_emplace_back(filename, std::move(ptr))) || using_directory;
				}

				if (!using_directory)
				{
					decrement_alias(alias);
				}
			}
		}

		void add_scan_watch(engine::file::watch_impl & /*impl*/, engine::Token id, ext::heap_shared_ptr<ScanData> ptr, bool recurse_directories)
		{
			if (!debug_verify(watches.emplace<ScanWatch>(id, ptr, recurse_directories)))
				return;

			ful::view_utfw filepath(ptr->dirpath);
			const auto alias = make_asset(filepath, recurse_directories);
			if (WatchData * const watch_data = get_or_create_directory(alias, filepath, recurse_directories))
			{
				if (!debug_verify(watch_data->scans.try_emplace_back(ptr)))
				{
					decrement_alias(alias);
				}
			}
		}

		void remove_watch(engine::file::watch_impl & /*impl*/, engine::Token id)
		{
			const auto watch_it = find(watches, id);
			if (!debug_verify(watch_it != watches.end()))
				return;

			watches.call(
				watch_it,
				[](const ReadWatch & x)
			{
				const auto directory_split = ful::rfind(x.ptr->filepath, ful::char16{'\\'});
				if (!debug_assert(directory_split != x.ptr->filepath.end()))
					return;

				ful::view_utfw filepath(x.ptr->filepath.begin(), directory_split + 1); // include '\\'
				const auto alias = make_asset(filepath, false);
				if (WatchData * const watch_data = get_directory(alias))
				{
					ful::view_utfw filename(directory_split + 1, x.ptr->filepath.end());

					bool using_directory = false;

					const auto report_missing_it = ext::find_if(watch_data->missing_reads, fun::second == x.ptr);
					if (report_missing_it != watch_data->missing_reads.end())
					{
						using_directory = true;
						watch_data->missing_reads.erase(report_missing_it);
					}

					const auto read_it = ext::find_if(watch_data->reads, fun::second == x.ptr);
					if (debug_verify(read_it != watch_data->reads.end()))
					{
						using_directory = true;
						watch_data->reads.erase(read_it);
					}

					if (using_directory)
					{
						decrement_alias(alias);
					}
				}
			},
				[](const ScanWatch & x)
			{
				ful::view_utfw filepath(x.ptr->dirpath);
				const auto alias = make_asset(filepath, x.recurse_directories);
				if (WatchData * const watch_data = get_directory(alias))
				{
					const auto scan_it = ext::find(watch_data->scans, x.ptr);
					if (debug_verify(scan_it != watch_data->scans.end()))
					{
						watch_data->scans.erase(scan_it);
						decrement_alias(alias);
					}
				}
			});

			watches.erase(watch_it);
		}
	}
}

#endif
