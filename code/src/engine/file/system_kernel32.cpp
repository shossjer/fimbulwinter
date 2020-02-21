#include "config.h"

#if FILE_USE_KERNEL32

#include "system.hpp"

#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"

#include <Windows.h>

namespace
{
	using watch_id = int;

	struct watch_callback
	{
		engine::file::watch_callback * callback;
		utility::any data;

		watch_callback(engine::file::watch_callback * callback, utility::any && data)
			: callback(callback)
			, data(std::move(data))
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

	watch_id add_watch(engine::file::watch_callback * callback, utility::any && data)
	{
		static watch_id next_id = 1; // reserve 0 (for no particular reason)
		const watch_id id = next_id++;

		watch_ids.push_back(id);
		watch_callbacks.emplace_back(callback, std::move(data));

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
			HANDLE hDirectory;
			OVERLAPPED overlapped;
			std::aligned_storage_t<1024, alignof(DWORD)> buffer; // arbitrary

			~async_type()
			{
				debug_verify(::CloseHandle(hDirectory) != FALSE, "failed with last error ", ::GetLastError());
			}
			async_type(HANDLE hDirectory, ext::index directory_index)
				: hDirectory(hDirectory)
			{
				overlapped.hEvent = reinterpret_cast<HANDLE>(directory_index);
			}
		};

		utility::heap_string_utfw filepath;
		bool temporary;
		int alias_count;

		std::unique_ptr<async_type> async;

		match fulls;
		match names;
		match extensions;

		directory_meta(utility::heap_string_utfw && filepath, bool temporary)
			: filepath(std::move(filepath))
			, temporary(temporary)
			, alias_count(0)
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

		if (!debug_assert(filepath.back() == L'\\'))
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

	bool try_read(utility::heap_string_utfw && filepath, watch_id watch_id, engine::Asset match)
	{
		const auto watch_index = find_watch(watch_id); // todo do this outside
		if (!debug_assert(watch_index != ext::index_invalid))
			return false;

		HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
		if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"failed with last error ", ::GetLastError()))
			return false;

		core::ReadStream stream([](void * dest, ext::usize n, void * data)
		                        {
			                        HANDLE hFile = reinterpret_cast<HANDLE>(data);

			                        DWORD read;
			                        if (!debug_verify(::ReadFile(hFile, dest, n, &read, nullptr) != FALSE, "failed with last error ", ::GetLastError()))
				                        return ext::ssize(-1);

			                        return ext::ssize(read);
		                        },
		                        reinterpret_cast<void *>(hFile),
		                        utility::heap_narrow<utility::encoding_utf8>(filepath));

		auto & watch_callback = watch_callbacks[watch_index];
		watch_callback.callback(std::move(stream), watch_callback.data, match);

		debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());
		return true;
	}

	ext::ssize scan_directory(const directory_meta & directory_meta)
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

		do
		{
			const auto filename = utility::string_view_utfw(data.cFileName);

			if (!debug_verify(utility::try_narrow(filename, match_name)))
				continue;

			const engine::Asset full_asset(match_name.data(), match_name.size());
			const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
			if (maybe_full != directory_meta.fulls.assets.end())
			{
				const auto match_index = maybe_full - directory_meta.fulls.assets.begin();

				number_of_matches++;
				try_read(directory_meta.filepath + filename, directory_meta.fulls.watches[match_index], full_asset);
				continue;
			}

			const auto dot = utility::unit_difference(match_name.rfind('.')).get();
			if (dot == ext::index_invalid)
				return false; // not eligible for partial matching

			const engine::Asset name_asset(match_name.data(), dot);
			const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
			if (maybe_name != directory_meta.names.assets.end())
			{
				const auto match_index = maybe_name - directory_meta.names.assets.begin();

				number_of_matches++;
				try_read(directory_meta.filepath + filename, directory_meta.names.watches[match_index], name_asset);
				continue;
			}

			const engine::Asset extension_asset(match_name.data() + dot, match_name.size() - dot);
			const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
			if (maybe_extension != directory_meta.extensions.assets.end())
			{
				const auto match_index = maybe_extension - directory_meta.extensions.assets.begin();

				number_of_matches++;
				try_read(directory_meta.filepath + filename, directory_meta.extensions.watches[match_index], extension_asset);
				continue;
			}
		}
		while (::FindNextFileW(hFile, &data) != FALSE);

		const auto error = ::GetLastError();
		debug_verify(error == ERROR_NO_MORE_FILES, "FindNextFileW failed with last error ", error);

		debug_verify(::FindClose(hFile) != FALSE, "failed with last error ", ::GetLastError());
		return number_of_matches;
	}

	ext::ssize scan_directory(const directory_meta & directory_meta, watch_id watch_id)
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

		do
		{
			const auto filename = utility::string_view_utfw(data.cFileName);

			if (!debug_verify(utility::try_narrow(filename, match_name)))
				continue;

			const engine::Asset full_asset(match_name.data(), match_name.size());
			const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
			if (maybe_full != directory_meta.fulls.assets.end())
			{
				const auto match_index = maybe_full - directory_meta.fulls.assets.begin();
				if (directory_meta.fulls.watches[match_index] != watch_id)
					continue;

				number_of_matches++;
				try_read(directory_meta.filepath + filename, watch_id, full_asset);
				continue;
			}

			const auto dot = utility::unit_difference(match_name.rfind('.')).get();
			if (dot == ext::index_invalid)
				return false; // not eligible for partial matching

			const engine::Asset name_asset(match_name.data(), dot);
			const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
			if (maybe_name != directory_meta.names.assets.end())
			{
				const auto match_index = maybe_name - directory_meta.names.assets.begin();
				if (directory_meta.names.watches[match_index] != watch_id)
					continue;

				number_of_matches++;
				try_read(directory_meta.filepath + filename, watch_id, name_asset);
				continue;
			}

			const engine::Asset extension_asset(match_name.data() + dot, match_name.size() - dot);
			const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
			if (maybe_extension != directory_meta.extensions.assets.end())
			{
				const auto match_index = maybe_extension - directory_meta.extensions.assets.begin();
				if (directory_meta.extensions.watches[match_index] != watch_id)
					continue;

				number_of_matches++;
				try_read(directory_meta.filepath + filename, watch_id, extension_asset);
				continue;
			}
		}
		while (::FindNextFileW(hFile, &data) != FALSE);

		const auto error = ::GetLastError();
		debug_verify(error == ERROR_NO_MORE_FILES, "FindNextFileW failed with last error ", error);

		debug_verify(::FindClose(hFile) != FALSE, "failed with last error ", ::GetLastError());
		return number_of_matches;
	}

	void CALLBACK Completion(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);

	bool try_read_directory(directory_meta::async_type & async)
	{
		DWORD notify_filter = FILE_NOTIFY_CHANGE_LAST_WRITE;
//#if MODE_DEBUG
//		notify_filter |= FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_SECURITY;
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

		auto async = std::make_unique<directory_meta::async_type>(hDirectory, directory_index);

		if (!try_read_directory(*async))
			return false;

		directory_metas[directory_index].async = std::move(async);

		return true;
	}

	void stop_watch(ext::index directory_index)
	{
		if (!debug_assert(directory_metas[directory_index].alias_count <= 0))
			return;

		if (!debug_assert(directory_metas[directory_index].async))
			return;

		debug_printline("stopping watch of \"", utility::heap_narrow<utility::encoding_utf8>(directory_metas[directory_index].filepath), "\"");
		debug_verify(::CancelIoEx(directory_metas[directory_index].async->hDirectory, &directory_metas[directory_index].async->overlapped) != FALSE, "failed with last error ", ::GetLastError());
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
		while (true)
		{
			aliases_it = std::find(aliases_it, from.aliases.end(), alias);
			if (aliases_it == from.aliases.end())
				break;

			const auto match_index = aliases_it - from.aliases.begin();
			to.assets.push_back(std::move(from.assets[match_index]));
			to.aliases.push_back(alias);
			to.watches.push_back(std::move(from.watches[match_index]));

			const auto last_index = from.aliases.size() - 1;
			from.assets[match_index] = std::move(from.assets[last_index]);
			from.aliases[match_index] = std::move(from.aliases[last_index]);
			from.watches[match_index] = std::move(from.watches[last_index]);
			from.assets.pop_back();
			from.aliases.pop_back();
			from.watches.pop_back();
		}
	}

	void move_matches(ext::index from_directory, ext::index to_directory, engine::Asset alias)
	{
		move_matches(directory_metas[from_directory].fulls, directory_metas[to_directory].fulls, alias);
		move_matches(directory_metas[from_directory].names, directory_metas[to_directory].names, alias);
		move_matches(directory_metas[from_directory].extensions, directory_metas[to_directory].extensions, alias);
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

	bool try_get_fullpath(utility::string_view_utfw filepath, utility::heap_string_utfw & fullpath)
	{
		const bool missing_prefix = !(filepath.data()[0] == L'\\' && filepath.data()[1] == L'\\' && filepath.data()[2] == L'?' && filepath.data()[3] == L'\\');
		const bool missing_postfix = filepath.back() != L'/';

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
		const DWORD directory_index = static_cast<DWORD>(reinterpret_cast<ext::index>(lpOverlapped->hEvent));

		if (dwErrorCode == ERROR_OPERATION_ABORTED ||
			!debug_assert(dwErrorCode == 0) ||
			!debug_assert(0 < dwNumberOfBytesTransfered))
		{
			directory_metas[directory_index].async.reset();
			return;
		}

		const DWORD size = (dwNumberOfBytesTransfered + (alignof(DWORD) - 1)) & ~(alignof(DWORD) - 1);
		debug_assert(size <= sizeof directory_metas[directory_index].async->buffer);
		const DWORD chunk_size = sizeof(DWORD) + sizeof(DWORD) + size;

		debug_assert(changes_buffer.size() % alignof(DWORD) == 0);
		changes_buffer.insert(changes_buffer.end(), reinterpret_cast<const char *>(&chunk_size), reinterpret_cast<const char *>(&chunk_size) + sizeof(DWORD));
		changes_buffer.insert(changes_buffer.end(), reinterpret_cast<const char *>(&directory_index), reinterpret_cast<const char *>(&directory_index) + sizeof(DWORD));

		const char * const buffer = reinterpret_cast<const char *>(&directory_metas[directory_index].async->buffer);
		changes_buffer.insert(changes_buffer.end(), buffer, buffer + size);

		debug_verify(::SetEvent(hEvent) != FALSE, "failed with last error ", ::GetLastError());

		if (!try_read_directory(*directory_metas[directory_index].async))
		{
			directory_metas[directory_index].async.reset();
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
				const ext::index directory_index = *reinterpret_cast<const DWORD *>(buffer + sizeof(DWORD));

				std::size_t offset = sizeof(DWORD) + sizeof(DWORD);
				bool chunk_done = false;
				while (!chunk_done)
				{
					debug_assert(offset % alignof(DWORD) == 0);

					const FILE_NOTIFY_INFORMATION * const info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(buffer + offset);
					debug_assert(info->FileNameLength % sizeof(wchar_t) == 0);

					chunk_done = info->NextEntryOffset == 0;
					offset += info->NextEntryOffset;

					const utility::string_view_utfw filename(info->FileName, utility::unit_difference(info->FileNameLength / sizeof(wchar_t)));

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

					if (info->Action != FILE_ACTION_MODIFIED)
						continue;
#endif

					const auto & directory_meta = directory_metas[directory_index];

					const engine::Asset full_asset(match_name.data(), match_name.size());
					const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
					if (maybe_full != directory_meta.fulls.assets.end())
					{
						const auto match_index = maybe_full - directory_meta.fulls.assets.begin();

						try_read(directory_meta.filepath + filename, directory_meta.fulls.watches[match_index], full_asset);
						continue;
					}

					const auto dot = utility::unit_difference(match_name.rfind('.')).get();
					if (dot == ext::index_invalid)
						continue; // not eligible for partial matching

					const engine::Asset name_asset(match_name.data(), dot);
					const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
					if (maybe_name != directory_meta.names.assets.end())
					{
						const auto match_index = maybe_name - directory_meta.names.assets.begin();

						try_read(directory_meta.filepath + filename, directory_meta.names.watches[match_index], name_asset);
						continue;
					}

					const engine::Asset extension_asset(match_name.data() + dot, match_name.size() - dot);
					const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
					if (maybe_extension != directory_meta.extensions.assets.end())
					{
						const auto match_index = maybe_extension - directory_meta.extensions.assets.begin();

						try_read(directory_meta.filepath + filename, directory_meta.extensions.watches[match_index], extension_asset);
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

			directory_meta directory_meta(utility::heap_string_utfw(directory_metas[directory_index].filepath), false);

			const auto watch_id = add_watch(x.callback, std::move(x.data)); // todo this feels like a hack

			auto from = utility::unit_difference(0);
			while (true)
			{
				auto found = utility::unit_difference(x.pattern.find('|', from));
				if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
				{
					if (x.pattern.data()[from.get()] == '*') // extension
					{
						const utility::string_view_utf8 extension(x.pattern.data() + from.get() + 1, found - from - 1); // ingnore '*'
						const engine::Asset asset(extension.data(), extension.size());

						directory_meta.extensions.assets.push_back(asset);
						directory_meta.extensions.aliases.push_back(x.directory);
						directory_meta.extensions.watches.push_back(watch_id);
					}
					else if (x.pattern.data()[found.get() - 1] == '*') // name
					{
						const utility::string_view_utf8 name(x.pattern.data() + from.get(), found - from - 1); // ingnore '*'
						const engine::Asset asset(name.data(), name.size());

						directory_meta.names.assets.push_back(asset);
						directory_meta.names.aliases.push_back(x.directory);
						directory_meta.names.watches.push_back(watch_id);
					}
					else // full
					{
						const utility::string_view_utf8 full(x.pattern.data() + from.get(), found - from);
						const engine::Asset asset(full.data(), full.size());

						directory_meta.fulls.assets.push_back(asset);
						directory_meta.fulls.aliases.push_back(x.directory);
						directory_meta.fulls.watches.push_back(watch_id);
					}
				}

				if (found == x.pattern.size())
					break; // done

				from = found + 1; // skip '|'
			}

			const auto number_of_matches = scan_directory(directory_meta, watch_id);
			if (number_of_matches == 0)
			{
				if (debug_assert(watch_ids.back() == watch_id))
				{
					auto & watch_callback = watch_callbacks.back();
					watch_callback.callback(core::ReadStream(nullptr, nullptr, ""), watch_callback.data, engine::Asset(""));
				}
			}

			remove_watch(watch_id);
		}
	};

	struct Watch
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;

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

			const auto watch_id = add_watch(x.callback, std::move(x.data));

			alias_metas[alias_index].watches.push_back(watch_id);

			auto from = utility::unit_difference(0);
			while (true)
			{
				auto found = utility::unit_difference(x.pattern.find('|', from));
				if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
				{
					if (x.pattern.data()[from.get()] == '*') // extension
					{
						const utility::string_view_utf8 extension(x.pattern.data() + from.get() + 1, found - from - 1); // ingnore '*'
						debug_printline("adding extension \"", extension, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const engine::Asset asset(extension.data(), extension.size());

						directory_meta.extensions.assets.push_back(asset);
						directory_meta.extensions.aliases.push_back(x.directory);
						directory_meta.extensions.watches.push_back(watch_id);
					}
					else if (x.pattern.data()[found.get() - 1] == '*') // name
					{
						const utility::string_view_utf8 name(x.pattern.data() + from.get(), found - from - 1); // ingnore '*'
						debug_printline("adding name \"", name, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const engine::Asset asset(name.data(), name.size());

						directory_meta.names.assets.push_back(asset);
						directory_meta.names.aliases.push_back(x.directory);
						directory_meta.names.watches.push_back(watch_id);
					}
					else // full
					{
						const utility::string_view_utf8 full(x.pattern.data() + from.get(), found - from);
						debug_printline("adding full \"", full, "\" to watch for \"", utility::heap_narrow<utility::encoding_utf8>(directory_meta.filepath), "\"");
						const engine::Asset asset(full.data(), full.size());

						directory_meta.fulls.assets.push_back(asset);
						directory_meta.fulls.aliases.push_back(x.directory);
						directory_meta.fulls.watches.push_back(watch_id);
					}
				}

				if (found == x.pattern.size())
					break; // done

				from = found + 1; // skip '|'
			}

			const bool should_watch =
				!directory_meta.fulls.assets.empty()
				|| !directory_meta.names.assets.empty()
				|| !directory_meta.extensions.assets.empty();
			if (debug_assert(should_watch, "nothing to watch in directory, please sanitize your data!"))
			{
				scan_directory(directory_meta, watch_id);

				if (!directory_metas[directory_index].async)
				{
					// note files that are created in between
					// the scan and the start of the watch will
					// not be reported
					try_start_watch(directory_index);
				}
			}
		}
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filename;
		engine::file::write_callback * callback;
		utility::any data;

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

			HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"failed with last error ", ::GetLastError()))
				return;

			core::WriteStream stream([](const void * src, ext::usize n, void * data)
										{
											HANDLE hFile = reinterpret_cast<HANDLE>(data);

											DWORD written;
											if (!debug_verify(::WriteFile(hFile, src, n, &written, nullptr) != FALSE, "failed with last error ", ::GetLastError()))
												return ext::ssize(-1);

											return ext::ssize(written);
										},
										reinterpret_cast<void *>(hFile),
										utility::heap_narrow<utility::encoding_utf8>(filepath));
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
			utility::any && data)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Read>(directory, std::move(pattern), callback, std::move(data));
			}
		}

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Watch>(directory, std::move(pattern), callback, std::move(data));
			}
		}

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data)
		{
			if (debug_assert(hThread != nullptr))
			{
				try_queue_apc<Write>(directory, std::move(filename), callback, std::move(data));
			}
		}
	}
}

#endif
