#include "config.h"

#if FILE_USE_KERNEL32

#include "system.hpp"

#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/string.hpp"

#include <Windows.h>

namespace
{
	using watch_id = int;

	struct watch_callback
	{
		engine::file::watch_callback * callback;
		utility::any data;

		engine::Asset alias;

		bool report_missing;
		bool once_only;

		watch_callback(engine::file::watch_callback * callback, utility::any && data, engine::Asset alias, bool report_missing, bool once_only)
			: callback(callback)
			, data(std::move(data))
			, alias(alias)
			, report_missing(report_missing)
			, once_only(once_only)
		{}
	};

	std::vector<watch_id> watch_ids;
	std::vector<watch_callback> watch_callbacks;

	ext::index find_watch(watch_id id)
	{
		const auto maybe = std::find(watch_ids.begin(), watch_ids.end(), id);
		if (maybe == watch_ids.end())
			return ext::index_invalid;

		return maybe - watch_ids.begin();
	}

	watch_id add_watch(engine::file::watch_callback * callback, utility::any && data, engine::Asset alias, bool report_missing, bool once_only)
	{
		static watch_id next_id = 1; // reserve 0 (for no particular reason)
		const watch_id id = next_id++;

		watch_ids.push_back(id);
		watch_callbacks.emplace_back(callback, std::move(data), alias, report_missing, once_only);

		return id;
	}

	void remove_watch(watch_id id)
	{
		const auto index = find_watch(id);
		if (!debug_assert(index != ext::index_invalid))
			return;

		watch_ids.erase(watch_ids.begin() + index);
		watch_callbacks.erase(watch_callbacks.begin() + index);
	}

	void clear_watches()
	{
		watch_ids.clear();
		watch_callbacks.clear();
	}

	struct directory_meta
	{
		struct match
		{
			std::vector<engine::Asset> assets;
			std::vector<engine::Asset> aliases;
			std::vector<watch_id> watches;
		};

		struct async_type
		{
			OVERLAPPED overlapped;
			HANDLE hDirectory;
			std::aligned_storage_t<1024, alignof(DWORD)> buffer; // arbitrary

			~async_type()
			{
				debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			}
			async_type(HANDLE hDirectory, engine::Asset filepath_asset)
				: hDirectory(hDirectory)
			{
				overlapped.hEvent = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(static_cast<engine::Asset::value_type>(filepath_asset)));
			}
		};
		static_assert(std::is_standard_layout<async_type>::value, "`async_type` and `OVERLAPPED` must be pointer-interconvertible");

		utility::heap_string_utfw filepath;
		bool temporary;
		int alias_count;

		// todo raw allocations are sketchy
		async_type * async;

		match fulls;
		match names;
		match extensions;

		directory_meta(utility::heap_string_utfw && filepath, bool temporary)
			: filepath(std::move(filepath))
			, temporary(temporary)
			, alias_count(0)
			, async(nullptr)
		{}
	};

	std::vector<engine::Asset> directory_filepath_assets; // weirdly casted from wchar_t
	std::vector<directory_meta> directory_metas;

	ext::index find_directory(engine::Asset filepath_asset)
	{
		const auto maybe = std::find(directory_filepath_assets.begin(), directory_filepath_assets.end(), filepath_asset);
		if (maybe != directory_filepath_assets.end())
			return maybe - directory_filepath_assets.begin();

		return ext::index_invalid;
	}

	struct temporary_directory { bool flag; /*explicit temporary_directory(bool flag) : flag(flag) {}*/ };
	ext::index find_or_add_directory(utility::heap_string_utfw && filepath, temporary_directory is_temporary = {false})
	{
		const engine::Asset asset(reinterpret_cast<const char *>(filepath.data()), filepath.size() * sizeof(wchar_t));

		const auto index = find_directory(asset);
		if (index != ext::index_invalid)
		{
			debug_assert(directory_metas[index].temporary == is_temporary.flag);
			return index;
		}

		const auto attributes = ::GetFileAttributesW(filepath.data());
		if (!debug_verify(attributes != INVALID_FILE_ATTRIBUTES, "GetFileAttributesW failed with last error ", ::GetLastError()))
			return ext::index_invalid;

		if (!debug_verify((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0, "trying to register a nondirectory"))
			return ext::index_invalid;

		directory_metas.emplace_back(std::move(filepath), is_temporary.flag);
		directory_filepath_assets.push_back(asset);

		return directory_filepath_assets.size() - 1;
	}

	void purge_temporary_directory(utility::heap_string_utfw & filepath)
	{
		debug_printline("removing temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

		if (!debug_assert((filepath.data()[0] == L'\\' && filepath.data()[1] == L'\\' && filepath.data()[2] == L'?' && filepath.data()[3] == L'\\')))
			return;

		if (!debug_assert(back(filepath) == L'\\'))
			return;

		filepath.data()[filepath.size() - 1] = L'\0';

		SHFILEOPSTRUCTW op =
		{
			nullptr,
			FO_DELETE,
			filepath.data() + 4, // skip "\\\\?\\"
			nullptr,
			FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT,
			FALSE,
			nullptr,
			nullptr
		};

		// todo this command is nasty
		const int ret = ::SHFileOperationW(&op);
		debug_verify((ret == 0 && op.fAnyOperationsAborted == FALSE), "SHFileOperationW failed with return value ", ret);

		// filepath.data()[filepath.size() - 1] = L'\\';
	}

	void remove_directory(ext::index index)
	{
		debug_assert(directory_metas[index].alias_count <= 0);

		if (directory_metas[index].temporary)
		{
			purge_temporary_directory(directory_metas[index].filepath);
		}

		directory_filepath_assets.erase(directory_filepath_assets.begin() + index);
		directory_metas.erase(directory_metas.begin() + index);
	}

	bool try_read(utility::heap_string_utfw && filepath, watch_callback & watch_callback, engine::Asset match)
	{
		if (!debug_assert((filepath.data()[0] == L'\\' && filepath.data()[1] == L'\\' && filepath.data()[2] == L'?' && filepath.data()[3] == L'\\')))
			return false;

		HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"failed with last error ", ::GetLastError()))
			return false;

		auto filepath_utf8 = utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filepath, 4)); // skip "\\\\?\\"
		std::replace(filepath_utf8.data(), filepath_utf8.data() + filepath_utf8.size(), '\\', '/');

		core::ReadStream stream([](void * dest, ext::usize n, void * data)
		                        {
			                        HANDLE hFile = reinterpret_cast<HANDLE>(data);

			                        DWORD read;
			                        if (!debug_verify(::ReadFile(hFile, dest, debug_cast<DWORD>(n), &read, nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				                        return ext::ssize(-1);

			                        return ext::ssize(read);
		                        },
		                        reinterpret_cast<void *>(hFile),
								std::move(filepath_utf8));

		watch_callback.callback(std::move(stream), watch_callback.data, match);

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());
		return true;
	}

	template <typename F>
	ext::ssize scan_directory(const directory_meta & directory_meta, F && f, ext::usize max = ext::usize(-1))
	{
		debug_printline("scanning directory \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");

		utility::heap_string_utfw pattern;
		if (!debug_verify(pattern.try_append(directory_meta.filepath)))
			return -1;
		if (!debug_verify(pattern.try_append(L'*')))
			return -1;

		WIN32_FIND_DATAW data;

		HANDLE hFile = ::FindFirstFileW(pattern.data(), &data);
		if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "FindFirstFileW failed with last error ", ::GetLastError()))
			return -1;

		ext::ssize number_of_matches = 0;

		utility::heap_string_utf8 match_name;

		bool break_early = false;
		do
		{
			const auto filename = utility::string_points_utfw(data.cFileName);

			if (!debug_verify(utility::try_narrow(filename, match_name)))
				continue;

			const auto full_asset = engine::Asset(match_name);
			const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
			if (maybe_full != directory_meta.fulls.assets.end())
			{
				if (f(filename, directory_meta.fulls, maybe_full - directory_meta.fulls.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
					{
						break_early = true;
						break;
					}
				}

				continue;
			}

			const auto dot = rfind(match_name, '.');
			if (dot == match_name.end())
				return false; // not eligible for partial matching

			const auto name_asset = engine::Asset(utility::string_units_utf8(match_name.begin(), dot));
			const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
			if (maybe_name != directory_meta.names.assets.end())
			{
				if (f(filename, directory_meta.names, maybe_name - directory_meta.names.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
					{
						break_early = true;
						break;
					}
				}

				continue;
			}

			const auto extension_asset = engine::Asset(utility::string_units_utf8(dot, match_name.end()));
			const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
			if (maybe_extension != directory_meta.extensions.assets.end())
			{
				if (f(filename, directory_meta.extensions, maybe_extension - directory_meta.extensions.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
					{
						break_early = true;
						break;
					}
				}

				continue;
			}
		}
		while (::FindNextFileW(hFile, &data) != FALSE);

		if (!break_early)
		{
			const auto error = ::GetLastError();
			debug_verify(error == ERROR_NO_MORE_FILES, "FindNextFileW failed with last error ", error);
		}

		debug_verify(::FindClose(hFile) != FALSE, "failed with last error ", ::GetLastError());
		return number_of_matches;
	}

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	bool try_read_directory(directory_meta::async_type & async)
	{
		DWORD notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
//#if MODE_DEBUG
//		notify_filter |= FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
//#endif
		return debug_verify(::ReadDirectoryChangesW(async.hDirectory, &async.buffer, sizeof async.buffer, FALSE, notify_filter, nullptr, &async.overlapped, Completion) != FALSE, "failed with last error ", ::GetLastError());
	}

	bool try_start_watch(ext::index directory_index)
	{
		if (!debug_assert(directory_metas[directory_index].alias_count > 0))
			return false;

		if (!debug_assert(!directory_metas[directory_index].async))
			return false;

		debug_printline("starting watch of \"", utility::heap_narrow<utility::encoding_utf8>(directory_metas[directory_index].filepath), "\"");
		HANDLE hDirectory = ::CreateFileW(
			directory_metas[directory_index].filepath.data(),
			FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			nullptr);
		if (!debug_verify(hDirectory != INVALID_HANDLE_VALUE, "CreateFileW failed with last error ", ::GetLastError()))
			return false;

		auto async = std::make_unique<directory_meta::async_type>(hDirectory, directory_filepath_assets[directory_index]);

		if (!try_read_directory(*async))
			return false;

		directory_metas[directory_index].async = async.release();

		return true;
	}

	void stop_watch(ext::index directory_index)
	{
		if (!debug_assert(directory_metas[directory_index].async))
			return;

		debug_printline("stopping watch of \"", utility::heap_narrow<utility::encoding_utf8>(directory_metas[directory_index].filepath), "\"");
		if (::CancelIoEx(directory_metas[directory_index].async->hDirectory, &directory_metas[directory_index].async->overlapped) == FALSE)
		{
			debug_verify(::GetLastError() == ERROR_NOT_FOUND, "CancelIoEx failed with last error ", ::GetLastError());
		}

		directory_metas[directory_index].async = nullptr;
		// note cleanup will happen in the completion function
	}

	void clear_directories()
	{
		for (auto index : ranges::index_sequence_for(directory_metas))
		{
			if (directory_metas[index].async)
			{
				stop_watch(index);
			}

			if (directory_metas[index].temporary)
			{
				purge_temporary_directory(directory_metas[index].filepath);
			}
		}

		directory_filepath_assets.clear();
		directory_metas.clear();
	}

	void move_matches(directory_meta::match & from, directory_meta::match & to, engine::Asset alias)
	{
		auto aliases_it = from.aliases.begin();
		auto aliases_end = from.aliases.end();
		while (true)
		{
			aliases_it = std::find(aliases_it, aliases_end, alias);
			if (aliases_it == aliases_end)
				break;

			const auto match_index = aliases_it - from.aliases.begin();
			to.assets.push_back(std::move(from.assets[match_index]));
			to.aliases.push_back(alias);
			to.watches.push_back(std::move(from.watches[match_index]));

			--aliases_end;
			const auto last_index = aliases_end - from.aliases.begin();
			from.assets[match_index] = std::move(from.assets[last_index]);
			from.aliases[match_index] = std::move(from.aliases[last_index]);
			from.watches[match_index] = std::move(from.watches[last_index]);
		}

		const auto remaining = aliases_end - from.aliases.begin();
		from.assets.erase(from.assets.begin() + remaining, from.assets.end());
		from.aliases.erase(from.aliases.begin() + remaining, from.aliases.end());
		from.watches.erase(from.watches.begin() + remaining, from.watches.end());
	}

	void move_matches(ext::index from_directory, ext::index to_directory, engine::Asset alias)
	{
		move_matches(directory_metas[from_directory].fulls, directory_metas[to_directory].fulls, alias);
		move_matches(directory_metas[from_directory].names, directory_metas[to_directory].names, alias);
		move_matches(directory_metas[from_directory].extensions, directory_metas[to_directory].extensions, alias);
	}

	void remove_matches(directory_meta::match & match, watch_id watch)
	{
		auto watch_it = match.watches.begin();
		auto watch_end = match.watches.end();
		while (true)
		{
			watch_it = std::find(watch_it, watch_end, watch);
			if (watch_it == watch_end)
				break;

			--watch_end;
			const auto match_index = watch_it - match.watches.begin();
			const auto last_index = watch_end - match.watches.begin();
			match.assets[match_index] = std::move(match.assets[last_index]);
			match.aliases[match_index] = std::move(match.aliases[last_index]);
			match.watches[match_index] = std::move(match.watches[last_index]);
		}

		const auto remaining = watch_end - match.watches.begin();
		match.assets.erase(match.assets.begin() + remaining, match.assets.end());
		match.aliases.erase(match.aliases.begin() + remaining, match.aliases.end());
		match.watches.erase(match.watches.begin() + remaining, match.watches.end());
	}

	void remove_matches(ext::index index, watch_id watch)
	{
		remove_matches(directory_metas[index].fulls, watch);
		remove_matches(directory_metas[index].names, watch);
		remove_matches(directory_metas[index].extensions, watch);
	}

	struct alias_meta
	{
		engine::Asset filepath_asset;

		std::vector<watch_id> watches;

		alias_meta(engine::Asset filepath_asset)
			: filepath_asset(filepath_asset)
		{}
	};

	std::vector<engine::Asset> alias_assets;
	std::vector<alias_meta> alias_metas;

	ext::index find_alias(engine::Asset asset)
	{
		const auto maybe = std::find(alias_assets.begin(), alias_assets.end(), asset);
		if (maybe != alias_assets.end())
			return maybe - alias_assets.begin();

		return ext::index_invalid;
	}

	ext::index add_alias(engine::Asset asset, engine::Asset filepath_asset)
	{
		alias_metas.emplace_back(filepath_asset);
		alias_assets.push_back(asset);

		return alias_assets.size() - 1;
	}

	void remove_alias(ext::index index)
	{
		for (auto id : alias_metas[index].watches)
		{
			remove_watch(id);
		}

		alias_assets.erase(alias_assets.begin() + index);
		alias_metas.erase(alias_metas.begin() + index);
	}

	void clear_aliases()
	{
		alias_assets.clear();
		alias_metas.clear();
	}

	void bind_alias_to_directory(engine::Asset alias, ext::index directory_index)
	{
		const auto alias_index = find_alias(alias);
		if (alias_index == ext::index_invalid)
		{
			add_alias(alias, directory_filepath_assets[directory_index]);
			directory_metas[directory_index].alias_count++;
		}
		else
		{
			const auto previous_directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			debug_assert(previous_directory_index != ext::index_invalid);

			alias_metas[alias_index].filepath_asset = directory_filepath_assets[directory_index];
			directory_metas[directory_index].alias_count++;

			move_matches(previous_directory_index, directory_index, alias);

			if (--directory_metas[previous_directory_index].alias_count <= 0)
			{
				if (directory_metas[previous_directory_index].async)
				{
					stop_watch(previous_directory_index);
				}
				remove_directory(previous_directory_index);
			}
		}
	}

	void remove_watch(engine::Asset alias, watch_id watch)
	{
		const auto alias_index = find_alias(alias);
		if (!debug_assert(alias_index != ext::index_invalid))
			return;

		auto maybe_watch = std::find(alias_metas[alias_index].watches.begin(), alias_metas[alias_index].watches.end(), watch);
		if (!debug_assert(maybe_watch != alias_metas[alias_index].watches.end()))
			return;

		maybe_watch = alias_metas[alias_index].watches.erase(maybe_watch);

		debug_assert(std::find(maybe_watch, alias_metas[alias_index].watches.end(), watch) == alias_metas[alias_index].watches.end());
	}

	bool try_get_fullpath(utility::string_units_utfw filepath, utility::heap_string_utfw & fullpath)
	{
		const bool missing_prefix = !(filepath.data()[0] == L'\\' && filepath.data()[1] == L'\\' && filepath.data()[2] == L'?' && filepath.data()[3] == L'\\');
		const bool missing_postfix = filepath.data()[filepath.size() - 1] != L'/';

		const auto len = ::GetFullPathNameW(filepath.data(), 0, nullptr, nullptr);
		if (!debug_verify(len != 0, "GetFullPathNameW failed with last error ", ::GetLastError()))
			return false;

		const ext::usize filepath_len = (missing_prefix ? 4 : 0) + len - 1 + (missing_postfix ? 1 : 0); // excluding null terminator
		if (!fullpath.try_resize(filepath_len))
			return false;

		if (missing_prefix)
		{
			fullpath.data()[0] = L'\\';
			fullpath.data()[1] = L'\\';
			fullpath.data()[2] = L'?';
			fullpath.data()[3] = L'\\';
		}

		const ext::usize fullpath_offset = (missing_prefix ? 4 : 0);
		const auto ret = ::GetFullPathNameW(filepath.data(), len, fullpath.data() + fullpath_offset, nullptr);
		if (!debug_verify(ret != 0, "GetFullPathNameW failed with last error ", ::GetLastError()))
			return false;

		if (missing_postfix)
		{
			const ext::usize postfix_offset = (missing_prefix ? 4 : 0) + len - 1; // excluding null terminator
			fullpath.data()[postfix_offset] = L'\\';
		}

		return true;
	}

	std::vector<char> changes_buffer;
	HANDLE hEvent = nullptr;

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
	{
		auto async = std::unique_ptr<directory_meta::async_type>(reinterpret_cast<directory_meta::async_type *>(lpOverlapped));
		// todo lpOverlapped->hEvent is superfluous
		const auto filepath_asset = engine::Asset(static_cast<engine::Asset::value_type>(reinterpret_cast<ext::index>(lpOverlapped->hEvent)));

		if (dwErrorCode != 0)
		{
			debug_assert(dwErrorCode == ERROR_OPERATION_ABORTED);
			debug_assert(0 == dwNumberOfBytesTransfered);

			return;
		}

		const DWORD size = (dwNumberOfBytesTransfered + (alignof(DWORD) - 1)) & ~(alignof(DWORD) - 1);
		debug_assert(size <= sizeof async->buffer);
		const DWORD chunk_size = sizeof(DWORD) + sizeof(DWORD) + size;

		debug_assert(changes_buffer.size() % alignof(DWORD) == 0);
		changes_buffer.insert(changes_buffer.end(), reinterpret_cast<const char *>(&chunk_size), reinterpret_cast<const char *>(&chunk_size) + sizeof(DWORD));
		static_assert(sizeof(engine::Asset::value_type) <= sizeof(DWORD), "engine::Asset cannot be packed in a DWORD");
		const DWORD filepath_asset_ = filepath_asset;
		changes_buffer.insert(changes_buffer.end(), reinterpret_cast<const char *>(&filepath_asset_), reinterpret_cast<const char *>(&filepath_asset_) + sizeof(DWORD));

		const char * const buffer = reinterpret_cast<const char *>(&async->buffer);
		changes_buffer.insert(changes_buffer.end(), buffer, buffer + size);

		debug_verify(::SetEvent(hEvent) != FALSE, "failed with last error ", ::GetLastError());

		const auto directory_index = find_directory(filepath_asset);
		if (directory_index == ext::index_invalid)
			return; // the directory has been removed

		if (directory_metas[directory_index].async != async.get())
			return; // the watch has stopped

		if (try_read_directory(*async))
		{
			async.release();
		}
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
				const std::size_t chunk_size = *reinterpret_cast<const DWORD *>(buffer);
				const auto directory_index = find_directory(engine::Asset(*reinterpret_cast<const DWORD *>(buffer + sizeof(DWORD))));

				if (directory_index == ext::index_invalid)
				{
					buffer += chunk_size;

					continue;
				}

				std::size_t offset = sizeof(DWORD) + sizeof(DWORD);
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
					if (!debug_verify(utility::try_narrow(filename, match_name)))
						continue;

#if MODE_DEBUG
					switch (info->Action)
					{
					case FILE_ACTION_ADDED:
						debug_printline("change FILE_ACTION_ADDED: ", match_name);
						break;
					case FILE_ACTION_REMOVED:
						debug_printline("change FILE_ACTION_REMOVED: ", match_name);
						break;
					case FILE_ACTION_MODIFIED:
						debug_printline("change FILE_ACTION_MODIFIED: ", match_name);
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
#endif

					const auto & directory_meta = directory_metas[directory_index];

					const auto full_asset = engine::Asset(match_name);
					const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
					if (maybe_full != directory_meta.fulls.assets.end())
					{
						const auto match_index = maybe_full - directory_meta.fulls.assets.begin();
						const auto watch_id = directory_meta.fulls.watches[match_index];
						const auto watch_index = find_watch(watch_id);
						if (debug_assert(watch_index != ext::index_invalid))
						{
							auto & watch_callback = watch_callbacks[watch_index];

							bool handled = false;

							if (info->Action == FILE_ACTION_MODIFIED)
							{
								try_read(directory_meta.filepath + filename, watch_callback, full_asset);
								handled = true;
							}

							if (info->Action == FILE_ACTION_REMOVED && watch_callback.report_missing)
							{
								const auto number_of_matches = scan_directory(
									directory_meta,
									[watch_id](utility::string_points_utfw /*filename*/, const struct directory_meta::match & match, ext::index match_index)
								{
									return match.watches[match_index] == watch_id;
								});
								if (number_of_matches == 0)
								{
									watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
									handled = true;
								}
							}

							if (handled && watch_callback.once_only)
							{
								remove_matches(directory_index, watch_id);
								if (directory_meta.fulls.aliases.empty() &&
									directory_meta.names.aliases.empty() &&
									directory_meta.extensions.aliases.empty())
								{
									stop_watch(directory_index);
								}
								remove_watch(watch_callback.alias, watch_id);
								remove_watch(watch_id);
							}
						}
						continue;
					}

					const auto dot = rfind(match_name, '.');
					if (dot == match_name.end())
						continue; // not eligible for partial matching

					const auto name_asset = engine::Asset(utility::string_units_utf8(match_name.begin(), dot));
					const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
					if (maybe_name != directory_meta.names.assets.end())
					{
						const auto match_index = maybe_name - directory_meta.names.assets.begin();
						const auto watch_id = directory_meta.names.watches[match_index];
						const auto watch_index = find_watch(watch_id);
						if (debug_assert(watch_index != ext::index_invalid))
						{
							auto & watch_callback = watch_callbacks[watch_index];

							bool handled = false;

							if (info->Action == FILE_ACTION_MODIFIED)
							{
								try_read(directory_meta.filepath + filename, watch_callback, name_asset);
								handled = true;
							}

							if (info->Action == FILE_ACTION_REMOVED && watch_callback.report_missing)
							{
								const auto number_of_matches = scan_directory(
									directory_meta,
									[watch_id](utility::string_points_utfw /*filename*/, const struct directory_meta::match & match, ext::index match_index)
								{
									return match.watches[match_index] == watch_id;
								});
								if (number_of_matches == 0)
								{
									watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
									handled = true;
								}
							}

							if (handled && watch_callback.once_only)
							{
								remove_matches(directory_index, watch_id);
								if (directory_meta.fulls.aliases.empty() &&
									directory_meta.names.aliases.empty() &&
									directory_meta.extensions.aliases.empty())
								{
									stop_watch(directory_index);
								}
								remove_watch(watch_callback.alias, watch_id);
								remove_watch(watch_id);
							}
						}
						continue;
					}

					const auto extension_asset = engine::Asset(utility::string_units_utf8(dot, match_name.end()));
					const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
					if (maybe_extension != directory_meta.extensions.assets.end())
					{
						const auto match_index = maybe_extension - directory_meta.extensions.assets.begin();
						const auto watch_id = directory_meta.extensions.watches[match_index];
						const auto watch_index = find_watch(watch_id);
						if (debug_assert(watch_index != ext::index_invalid))
						{
							auto & watch_callback = watch_callbacks[watch_index];

							bool handled = false;

							if (info->Action == FILE_ACTION_MODIFIED)
							{
								try_read(directory_meta.filepath + filename, watch_callback, extension_asset);
								handled = true;
							}

							if (info->Action == FILE_ACTION_REMOVED && watch_callback.report_missing)
							{
								const auto number_of_matches = scan_directory(
									directory_meta,
									[watch_id](utility::string_points_utfw /*filename*/, const struct directory_meta::match & match, ext::index match_index)
								{
									return match.watches[match_index] == watch_id;
								});
								if (number_of_matches == 0)
								{
									watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
									handled = true;
								}
							}

							if (handled && watch_callback.once_only)
							{
								remove_matches(directory_index, watch_id);
								if (directory_meta.fulls.aliases.empty() &&
									directory_meta.names.aliases.empty() &&
									directory_meta.extensions.aliases.empty())
								{
									stop_watch(directory_index);
								}
								remove_watch(watch_callback.alias, watch_id);
								remove_watch(watch_id);
							}
						}
						continue;
					}
				}

				buffer += chunk_size;
			}

			debug_verify(::ResetEvent(hEvent) != FALSE, "failed with last error ", ::GetLastError());
			changes_buffer.clear();
		}

		return 0;
	}

	struct RegisterDirectory
	{
		engine::Asset alias;
		utility::heap_string_utf8 filepath;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<RegisterDirectory> data(reinterpret_cast<RegisterDirectory *>(Parameter));
			RegisterDirectory & x = *data;

			utility::heap_string_utfw filepath;
			if (!try_get_fullpath(utility::heap_widen(x.filepath), filepath))
				return;

			const auto directory_index = find_or_add_directory(std::move(filepath));
			if (!debug_verify(directory_index != ext::index_invalid))
				return;

			bind_alias_to_directory(x.alias, directory_index);
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
			if (!try_get_fullpath(tempfilepath, filepath))
				return;

			if (!debug_verify(::CreateDirectoryW(filepath.data(), nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				return;

			debug_printline("created temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

			const auto directory_index = find_or_add_directory(std::move(filepath), temporary_directory{true});
			if (!debug_verify(directory_index != ext::index_invalid, "CreateDirectoryW lied to us!"))
			{
				purge_temporary_directory(filepath);
				return;
			}

			bind_alias_to_directory(x.alias, directory_index);
		}
	};

	struct UnregisterDirectory
	{
		engine::Asset alias;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<UnregisterDirectory> data(reinterpret_cast<UnregisterDirectory *>(Parameter));
			UnregisterDirectory & x = *data;

			auto alias_index = find_alias(x.alias);
			if (!debug_verify(alias_index != ext::index_invalid, "directory alias does not exist"))
				return;

			const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			if (!debug_assert(directory_index != ext::index_invalid))
				return;

			remove_alias(alias_index);

			if (--directory_metas[directory_index].alias_count <= 0)
			{
				if (directory_metas[directory_index].async)
				{
					stop_watch(directory_index);
				}
				remove_directory(directory_index);
			}
		}
	};

	struct Read
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Read> data(reinterpret_cast<Read *>(Parameter));
			Read & x = *data;

			const auto alias_index = find_alias(x.directory);
			if (!debug_verify(alias_index != ext::index_invalid))
				return;

			const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			if (!debug_assert(directory_index != ext::index_invalid))
				return;

			directory_meta temporary_meta(utility::heap_string_utfw(directory_metas[directory_index].filepath), false);

			debug_expression(bool added_any_match = false);
			auto from = x.pattern.begin();
			while (true)
			{
				auto found = find(from, x.pattern.end(), '|');
				if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
				{
					if (*from == '*') // extension
					{
						const auto extension = utility::string_units_utf8(from + 1, found); // ingnore '*'
						const auto asset = engine::Asset(extension);

						temporary_meta.extensions.assets.push_back(asset);
						debug_expression(added_any_match = true);
					}
					else if (*(found - 1) == '*') // name
					{
						const auto name = utility::string_units_utf8(from, found - 1); // ingnore '*'
						const auto asset = engine::Asset(name);

						temporary_meta.names.assets.push_back(asset);
						debug_expression(added_any_match = true);
					}
					else // full
					{
						const auto full = utility::string_units_utf8(from, found);
						const auto asset = engine::Asset(full);

						temporary_meta.fulls.assets.push_back(asset);
						debug_expression(added_any_match = true);
					}
				}

				if (found == x.pattern.end())
					break; // done

				from = found + 1; // skip '|'
			}
			debug_assert(added_any_match, "nothing to read in directory, please sanitize your data!");

			watch_callback watch_callback{x.callback, std::move(x.data), engine::Asset{}, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING), static_cast<bool>(x.mode & engine::file::flags::ONCE_ONLY)};

			const ext::usize number_of_matches = scan_directory(
				temporary_meta,
				[&temporary_meta, &watch_callback](utility::string_points_utfw filename, const struct directory_meta::match & match, ext::index match_index)
			{
				try_read(temporary_meta.filepath + filename, watch_callback, match.assets[match_index]);
				return true;
			},
				watch_callback.once_only ? 1 : -1);
			if (watch_callback.report_missing && number_of_matches == 0)
			{
				watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
			}
		}
	};

	struct Remove
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Remove> data(reinterpret_cast<Remove *>(Parameter));
			Remove & x = *data;

			const auto alias_index = find_alias(x.directory);
			if (!debug_verify(alias_index != ext::index_invalid))
				return; // error

			const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			if (!debug_assert(directory_index != ext::index_invalid))
				return; // error

			auto & directory_meta = directory_metas[directory_index];

			scan_directory(
				directory_meta,
				[&directory_meta](utility::string_points_utfw filename, const struct directory_meta::match & /*match*/, ext::index /*match_index*/)
			{
				auto filepath = directory_meta.filepath + filename;
				debug_verify(::DeleteFileW(filepath.data()) != FALSE, "failed with last error ", ::GetLastError());
				return true;
			});
		}
	};

	struct Watch
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Watch> data(reinterpret_cast<Watch *>(Parameter));
			Watch & x = *data;

			const auto alias_index = find_alias(x.directory);
			if (!debug_verify(alias_index != ext::index_invalid))
				return;

			const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			if (!debug_assert(directory_index != ext::index_invalid))
				return;

			auto & directory_meta = directory_metas[directory_index];

			const auto watch_id = add_watch(x.callback, std::move(x.data), x.directory, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING), static_cast<bool>(x.mode & engine::file::flags::ONCE_ONLY));

			alias_metas[alias_index].watches.push_back(watch_id);

			debug_expression(bool added_any_match = false);
			auto from = x.pattern.begin();
			while (true)
			{
				auto found = find(from, x.pattern.end(), '|');
				if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
				{
					if (*from == '*') // extension
					{
						const auto extension = utility::string_units_utf8(from + 1, found); // ingnore '*'
						debug_printline("adding extension \"", extension, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const auto asset = engine::Asset(extension);

						directory_meta.extensions.assets.push_back(asset);
						directory_meta.extensions.aliases.push_back(x.directory);
						directory_meta.extensions.watches.push_back(watch_id);
						debug_expression(added_any_match = true);
					}
					else if (*(found - 1) == '*') // name
					{
						const auto name = utility::string_units_utf8(from, found - 1); // ingnore '*'
						debug_printline("adding name \"", name, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const auto asset = engine::Asset(name);

						directory_meta.names.assets.push_back(asset);
						directory_meta.names.aliases.push_back(x.directory);
						directory_meta.names.watches.push_back(watch_id);
						debug_expression(added_any_match = true);
					}
					else // full
					{
						const auto full = utility::string_units_utf8(from, found);
						debug_printline("adding full \"", full, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const auto asset = engine::Asset(full);

						directory_meta.fulls.assets.push_back(asset);
						directory_meta.fulls.aliases.push_back(x.directory);
						directory_meta.fulls.watches.push_back(watch_id);
						debug_expression(added_any_match = true);
					}
				}

				if (found == x.pattern.end())
					break; // done

				from = found + 1; // skip '|'
			}
			debug_assert(added_any_match, "nothing to read in directory, please sanitize your data!");

			if (!(x.mode & engine::file::flags::IGNORE_EXISTING))
			{
				if (!debug_assert(watch_ids.back() == watch_id))
					return;

				auto & watch_callback = watch_callbacks.back();

				ext::usize number_of_matches = scan_directory(
					directory_meta,
					[&directory_meta, &watch_callback, watch_id](utility::string_points_utfw filename, const struct directory_meta::match & match, ext::index match_index)
				{
					if (match.watches[match_index] != watch_id) // todo always newly added
						return false;

					try_read(directory_meta.filepath + filename, watch_callback, match.assets[match_index]);
					return true;
				},
					watch_callback.once_only ? 1 : -1);
				if (watch_callback.report_missing && number_of_matches == 0)
				{
					number_of_matches++;
					watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
				}

				if (watch_callback.once_only && number_of_matches >= 1)
				{
					remove_matches(directory_index, watch_id); // todo always the last entries
					if (debug_assert(alias_metas[alias_index].watches.back() == watch_id))
					{
						alias_metas[alias_index].watches.pop_back();
					}
					remove_watch(watch_id); // always last
					return;
				}
			}

			if (!directory_metas[directory_index].async)
			{
				// todo files that are created in between the
				// scan and the start of the watch will not be
				// reported, maybe watch before scan and
				// remove duplicates?
				try_start_watch(directory_index);
			}
		}
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filename;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;

		static void NTAPI Callback(ULONG_PTR Parameter)
		{
			std::unique_ptr<Write> data(reinterpret_cast<Write *>(Parameter));
			Write & x = *data;

			const auto alias_index = find_alias(x.directory);
			if (!debug_verify(alias_index != ext::index_invalid))
				return;

			const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
			if (!debug_assert(directory_index != ext::index_invalid))
				return;

			const auto & directory_meta = directory_metas[directory_index];

			auto filepath = directory_meta.filepath;
			if (!utility::try_widen_append(x.filename, filepath))
				return;

			if (!debug_assert((filepath.data()[0] == L'\\' && filepath.data()[1] == L'\\' && filepath.data()[2] == L'?' && filepath.data()[3] == L'\\')))
				return;

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

			auto filepath_utf8 = utility::heap_narrow<utility::encoding_utf8>(utility::string_points_utfw(filepath, 4)); // skip "\\\\?\\"
			std::replace(filepath_utf8.data(), filepath_utf8.data() + filepath_utf8.size(), '\\', '/');

			core::WriteStream stream([](const void * src, ext::usize n, void * data)
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

			debug_assert(active == false);
		}

		system::system()
		{
			if (!debug_assert(hThread == nullptr))
				return;

			if (!debug_assert(active == false))
				return;

			if (!debug_assert(hEvent == nullptr))
				return;

			hEvent = ::CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!debug_verify(hEvent != nullptr, "CreateEventW failed with last error ", ::GetLastError()))
				return;

			active = true;

			hThread = ::CreateThread(nullptr, 0, file_watch, nullptr, 0, nullptr);
			if (!debug_verify(hThread != nullptr, "CreateThread failed with last error ", ::GetLastError()))
			{
				active = false;
				debug_verify(::CloseHandle(hEvent) != FALSE, "failed with last error ", ::GetLastError());
				hEvent = nullptr;
			}
		}

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RegisterDirectory>(name, std::move(path));
			}
		}

		void register_temporary_directory(engine::Asset name)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<RegisterTemporaryDirectory>(name);
			}
		}

		void unregister_directory(engine::Asset name)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<UnregisterDirectory>(name);
			}
		}

		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Read>(directory, std::move(pattern), callback, std::move(data), mode);
			}
		}

		void remove(engine::Asset directory, utility::heap_string_utf8 && pattern)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Remove>(directory, std::move(pattern));
			}
		}

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Watch>(directory, std::move(pattern), callback, std::move(data), mode);
			}
		}

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Write>(directory, std::move(filename), callback, std::move(data), mode);
			}
		}
	}
}

#endif
