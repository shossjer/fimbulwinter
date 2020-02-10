#include "config.h"

#if FILE_USE_KERNEL32

#include "system.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/variant.hpp"

#include <Windows.h>

namespace
{
	struct RegisterDirectory
	{
		engine::Asset alias;
		utility::heap_string_utf8 filepath;
	};

	struct RegisterTemporaryDirectory
	{
		engine::Asset alias;
	};

	struct UnregisterDirectory
	{
		engine::Asset alias;
	};

	struct Read
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;
	};

	struct Watch
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filename;
		engine::file::write_callback * callback;
		utility::any data;
	};

	struct Terminate {};

	using Message = utility::variant<
		RegisterDirectory,
		RegisterTemporaryDirectory,
		UnregisterDirectory,
		Read,
		Watch,
		Write,
		Terminate
	>;

	core::container::PageQueue<utility::heap_storage<Message>> message_queue;
}

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

		utility::heap_string_utfw filepath;
		bool temporary;
		int alias_count;

		match fulls;
		match names;
		match extensions;

		directory_meta(utility::heap_string_utfw && filepath, bool temporary)
			: filepath(std::move(filepath))
			, temporary(temporary)
			, alias_count(0)
		{}
	};

	std::vector<engine::Asset> directory_filepath_assets;
	std::vector<HANDLE> directory_handles;
	std::vector<directory_meta> directory_metas;

	ext::index find_directory(engine::Asset filepath_asset)
	{
		const auto maybe = std::find(directory_filepath_assets.begin(), directory_filepath_assets.end(), filepath_asset);
		if (maybe != directory_filepath_assets.end())
			return maybe - directory_filepath_assets.begin();

		return ext::index_invalid;
	}

	ext::index find_directory(HANDLE hWatch)
	{
		const auto maybe = std::find(directory_handles.begin(), directory_handles.end(), hWatch);
		if (maybe != directory_handles.end())
			return maybe - directory_handles.begin();

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
		directory_handles.push_back(INVALID_HANDLE_VALUE);
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
		directory_handles.erase(directory_handles.begin() + index);
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

	bool try_start_watch(ext::index directory_index)
	{
		if (!debug_assert(directory_metas[directory_index].alias_count > 0))
			return false;

		if (!debug_assert(directory_handles[directory_index] == INVALID_HANDLE_VALUE))
			return false;

		DWORD notify_filter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
#if MODE_DEBUG
		notify_filter |= 0; // todo
#endif
		debug_printline("starting watch of \"", utility::heap_narrow<utility::encoding_utf8>(directory_metas[directory_index].filepath), "\"");
		HANDLE hWatch = ::FindFirstChangeNotificationW(directory_metas[directory_index].filepath.data(), FALSE, notify_filter);
		directory_handles[directory_index] = hWatch;
		return debug_verify(hWatch != INVALID_HANDLE_VALUE, "FindFirstChangeNotificationW failed with last error ", ::GetLastError());
	}

	void stop_watch(ext::index directory_index)
	{
		if (!debug_assert(directory_metas[directory_index].alias_count <= 0))
			return;

		if (!debug_assert(directory_handles[directory_index] != INVALID_HANDLE_VALUE))
			return;

		debug_printline("stopping watch of \"", utility::heap_narrow<utility::encoding_utf8>(directory_metas[directory_index].filepath), "\"");
		debug_verify(::FindCloseChangeNotification(directory_handles[directory_index]) != FALSE, "failed with last error ", ::GetLastError());
		directory_handles[directory_index] = INVALID_HANDLE_VALUE;
	}

	void clear_directories()
	{
		for (auto index : ranges::index_sequence_for(directory_metas))
		{
			if (directory_handles[index] != INVALID_HANDLE_VALUE)
			{
				stop_watch(index);
			}

			if (directory_metas[index].temporary)
			{
				purge_temporary_directory(directory_metas[index].filepath);
			}
		}

		directory_filepath_assets.clear();
		directory_handles.clear();
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

	void bind_alias_to_directory(engine::Asset alias, ext::index directory_index, std::vector<HANDLE> & handles)
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
				if (directory_handles[previous_directory_index] != INVALID_HANDLE_VALUE)
				{
					handles.erase(std::find(handles.begin(), handles.end(), directory_handles[previous_directory_index]));
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

	HANDLE hEvent;
	core::async::Thread thread;

	void file_watch()
	{
		std::vector<HANDLE> handles;

		handles.push_back(hEvent);

		auto panic =
			[&handles]()
			{
				clear_aliases();
				clear_directories();
				clear_watches();
			};

		while (true)
		{
			// todo support more than MAXIMUM_WAIT_OBJECTS objects, examples
			// are given on msdn
			//
			// https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitformultipleobjects
			if (!debug_verify(handles.size() <= MAXIMUM_WAIT_OBJECTS))
				return panic();

			const auto ret = ::WaitForMultipleObjects(handles.size(), handles.data(), FALSE, INFINITE);
			if (!debug_verify(ret != WAIT_FAILED, "WaitForMultipleObjects failed with last error ", ::GetLastError()))
				return panic();

			if (!debug_assert((WAIT_OBJECT_0 <= ret && ret < WAIT_OBJECT_0 + handles.size())))
				return panic();

			if (ret == WAIT_OBJECT_0)
			{
				if (!debug_assert(handles[0] == hEvent))
					return panic();

				debug_verify(::ResetEvent(hEvent) != 0, "failed with last error ", ::GetLastError());

				Message message;
				while (message_queue.try_pop(message))
				{
					if (utility::holds_alternative<Terminate>(message))
					{
						debug_assert(alias_assets.empty());
						debug_assert(directory_filepath_assets.empty());
						debug_assert(watch_ids.empty());
						return panic(); // can we really call it panic if it is intended?
					}

					struct
					{
						std::vector<HANDLE> & handles;

						void operator () (RegisterDirectory && x)
						{
							utility::heap_string_utfw filepath;
							if (!try_get_fullpath(utility::heap_widen(x.filepath), filepath))
								return; // error

							const auto directory_index = find_or_add_directory(std::move(filepath));
							if (!debug_verify(directory_index != ext::index_invalid))
								return; // error

							bind_alias_to_directory(x.alias, directory_index, handles);
						}

						void operator () (RegisterTemporaryDirectory && x)
						{
							utility::static_string_utfw<(MAX_PATH + 1 + 1)> temppath(MAX_PATH + 1); // GetTempPathW returns at most (MAX_PATH + 1) (not counting null terminator)
							if (!debug_verify(::GetTempPathW(MAX_PATH + 1 + 1, temppath.data()) != 0, "failed with last error ", ::GetLastError()))
								return; // error

							// todo resize temppath

							utility::static_string_utfw<MAX_PATH> tempfilepath(MAX_PATH - 1); // GetTempFileNameW returns at most MAX_PATH (including null terminator)
							if (!debug_verify(::GetTempFileNameW(temppath.data(), L"unn", 0, tempfilepath.data()) != 0, "failed with last error ", ::GetLastError()))
								return; // error

							// todo resize filepath

							if (!debug_verify(::DeleteFileW(tempfilepath.data()) != FALSE, "failed with last error ", ::GetLastError()))
								return; // error

							utility::heap_string_utfw filepath;
							if (!try_get_fullpath(tempfilepath, filepath))
								return; // error

							if (!debug_verify(::CreateDirectoryW(filepath.data(), nullptr) != FALSE, "failed with last error ", ::GetLastError()))
								return; // error

							debug_printline("created temporary directory \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"");

							const auto directory_index = find_or_add_directory(std::move(filepath), temporary_directory{true});
							if (!debug_verify(directory_index != ext::index_invalid, "CreateDirectoryW lied to us!"))
							{
								purge_temporary_directory(filepath);
								return; // error
							}

							bind_alias_to_directory(x.alias, directory_index, handles);
						}

						void operator () (UnregisterDirectory && x)
						{
							auto alias_index = find_alias(x.alias);
							if (!debug_verify(alias_index != ext::index_invalid, "directory alias does not exist"))
								return; // error

							const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
							if (!debug_assert(directory_index != ext::index_invalid))
								return; // error

							remove_alias(alias_index);

							if (--directory_metas[directory_index].alias_count <= 0)
							{
								if (directory_handles[directory_index] != INVALID_HANDLE_VALUE)
								{
									handles.erase(std::find(handles.begin(), handles.end(), directory_handles[directory_index]));
									stop_watch(directory_index);
								}
								remove_directory(directory_index);
							}
						}

						void operator () (Read && x)
						{
							const auto alias_index = find_alias(x.directory);
							if (!debug_verify(alias_index != ext::index_invalid))
								return; // error

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

						void operator () (Watch && x)
						{
							const auto alias_index = find_alias(x.directory);
							if (!debug_verify(alias_index != ext::index_invalid))
								return; // error

							const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
							if (!debug_assert(directory_index != ext::index_invalid))
								return; // error

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

								if (directory_handles[directory_index] == INVALID_HANDLE_VALUE)
								{
									// note files that are created in between
									// the scan and the start of the watch will
									// not be reported
									if (try_start_watch(directory_index))
									{
										handles.push_back(directory_handles[directory_index]);
									}
								}
							}
						}

						void operator () (Write && x)
						{
							const auto alias_index = find_alias(x.directory);
							if (!debug_verify(alias_index != ext::index_invalid))
								return; // error

							const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
							if (!debug_assert(directory_index != ext::index_invalid))
								return; // error

							const auto & directory_meta = directory_metas[directory_index];

							auto filepath = directory_meta.filepath;
							if (!utility::try_widen_append(x.filename, filepath))
								return; // error

							HANDLE hFile = ::CreateFileW(filepath.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
							if (!debug_verify(hFile != INVALID_HANDLE_VALUE, "CreateFileW \"", utility::heap_narrow<utility::encoding_utf8>(filepath), "\"failed with last error ", ::GetLastError()))
								return; // error

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

						void operator () (Terminate &&)
						{
							intrinsic_unreachable();
						}

					} visitor{handles};

					visit(visitor, std::move(message));
				}
			}
			else
			{
				std::aligned_storage_t<4096, alignof(DWORD)> buffer; // arbitrary

				const auto index = ret - WAIT_OBJECT_0;

				DWORD size;
				if (!debug_verify(::ReadDirectoryChangesW(handles[index], &buffer, sizeof buffer, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE, &size, nullptr, nullptr) != 0, "failed with last error ", ::GetLastError()))
					return panic();

				const auto directory_index = find_directory(handles[index]);

				if (size == 0)
				{
					scan_directory(directory_metas[directory_index]);
				}
				else
				{
					const char * cur = nullptr;
					const char * nxt = reinterpret_cast<const char *>(&buffer);
					while (cur != nxt)
					{
						const FILE_NOTIFY_INFORMATION * const info = reinterpret_cast<const FILE_NOTIFY_INFORMATION *>(nxt);
						if (!debug_assert(info->NextEntryOffset % alignof(DWORD) == 0))
							return panic();

						cur = nxt;
						nxt = nxt + info->NextEntryOffset;

#if MODE_DEBUG
						switch (info->Action)
						{
						case FILE_ACTION_ADDED:
							debug_printline("change FILE_ACTION_ADDED: ", utility::heap_narrow<utility::encoding_utf8>(info->FileName));
							break;
						case FILE_ACTION_REMOVED:
							debug_printline("change FILE_ACTION_REMOVED: ", utility::heap_narrow<utility::encoding_utf8>(info->FileName));
							break;
						case FILE_ACTION_MODIFIED:
							debug_printline("change FILE_ACTION_MODIFIED: ", utility::heap_narrow<utility::encoding_utf8>(info->FileName));
							break;
						case FILE_ACTION_RENAMED_OLD_NAME:
							debug_printline("change FILE_ACTION_RENAMED_OLD_NAME: ", utility::heap_narrow<utility::encoding_utf8>(info->FileName));
							break;
						case FILE_ACTION_RENAMED_NEW_NAME:
							debug_printline("change FILE_ACTION_RENAMED_NEW_NAME: ", utility::heap_narrow<utility::encoding_utf8>(info->FileName));
							break;
						default:
							debug_unreachable("unknown action ", info->Action);
						}
#endif
					}
				}

				if (!debug_verify(::FindNextChangeNotification(handles[index]) != FALSE, "failed with last error ", ::GetLastError()))
					return panic();
			}
		}
	}
}

namespace engine
{
	namespace file
	{
		system::~system()
		{
			if (!debug_verify(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Terminate>)))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}

			thread.join();

			debug_verify(::CloseHandle(hEvent) != FALSE, "failed with last error ", ::GetLastError());

			// if (auto size = message_queue.clear())
			// {
			// 	debug_printline("dropped ", size, " messages on exit");
			// }
		}

		system::system()
		{
			hEvent = ::CreateEventW(nullptr, TRUE, FALSE, NULL);
			if (!debug_verify(hEvent != nullptr, "CreateEventW failed with last error ", ::GetLastError()))
				return;

			thread = core::async::Thread(file_watch);
			if (!debug_verify(thread.valid()))
			{
				debug_verify(::CloseHandle(hEvent) != FALSE, "failed with last error ", ::GetLastError());
				return;
			}
		}

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<RegisterDirectory>, name, std::move(path))))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}

		void register_temporary_directory(engine::Asset name)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<RegisterTemporaryDirectory>, name)))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}

		void unregister_directory(engine::Asset name)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<UnregisterDirectory>, name)))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}

		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Read>, directory, std::move(pattern), callback, std::move(data))))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Watch>, directory, std::move(pattern), callback, std::move(data))))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Write>, directory, std::move(filename), callback, std::move(data))))
			{
				debug_verify(::SetEvent(hEvent) == TRUE, "failed with last error ", ::GetLastError());
			}
		}
	}
}

#endif
