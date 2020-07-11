#include "config.h"

#if FILE_USE_INOTIFY

#include "system.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/ext/unistd.hpp"
#include "utility/variant.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

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
		engine::file::flags mode;
	};

	struct Remove
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
	};

	struct Watch
	{
		engine::Asset directory;
		utility::heap_string_utf8 pattern;
		engine::file::watch_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct Write
	{
		engine::Asset directory;
		utility::heap_string_utf8 filename;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct Terminate {};

	using Message = utility::variant<
		RegisterDirectory,
		RegisterTemporaryDirectory,
		UnregisterDirectory,
		Read,
		Remove,
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

		utility::heap_string_utf8 filepath;
		bool temporary;
		int alias_count;

		match fulls;
		match names;
		match extensions;

		directory_meta(utility::heap_string_utf8 && filepath, bool temporary)
			: filepath(std::move(filepath))
			, temporary(temporary)
			, alias_count(0)
		{}
	};

	std::vector<engine::Asset> directory_filepath_assets;
	std::vector<int> directory_inotify_fds;
	std::vector<directory_meta> directory_metas;

	ext::index find_directory(engine::Asset filepath_asset)
	{
		const auto maybe = std::find(directory_filepath_assets.begin(), directory_filepath_assets.end(), filepath_asset);
		if (maybe != directory_filepath_assets.end())
			return maybe - directory_filepath_assets.begin();

		return ext::index_invalid;
	}

	ext::index find_directory(int fd)
	{
		const auto maybe = std::find(directory_inotify_fds.begin(), directory_inotify_fds.end(), fd);
		if (maybe != directory_inotify_fds.end())
			return maybe - directory_inotify_fds.begin();

		return ext::index_invalid;
	}

	struct temporary_directory { bool flag; /*explicit temporary_directory(bool flag) : flag(flag) {}*/ };
	ext::index find_or_add_directory(utility::heap_string_utf8 && filepath, temporary_directory is_temporary = {false})
	{
		const engine::Asset asset(filepath.data(), filepath.size());

		const auto index = find_directory(asset);
		if (index != ext::index_invalid)
		{
			debug_assert(directory_metas[index].temporary == is_temporary.flag);
			return index;
		}

		struct stat64 buf;
		if (!debug_verify(::stat64(filepath.data(), &buf) != -1, "failed with errno ", errno))
			return ext::index_invalid;

		if (!debug_verify(S_ISDIR(buf.st_mode), "trying to register a nondirectory"))
			return ext::index_invalid;

		directory_metas.emplace_back(std::move(filepath), is_temporary.flag);
		directory_inotify_fds.push_back(-1);
		directory_filepath_assets.push_back(asset);

		return directory_filepath_assets.size() - 1;
	}

	void purge_temporary_directory(const char * filepath)
	{
		debug_printline("removing temporary directory \"", filepath, "\"");
		debug_verify(::nftw(filepath,
		                    [](const char * filepath, const struct stat * /*sb*/, int type, struct FTW * /*ftwbuf*/)
		                    {
			                    switch (type)
			                    {
			                    case FTW_F:
			                    case FTW_SL:
				                    return debug_verify(::unlink(filepath) != -1, "failed with errno ", errno) ? 0 : -1;
			                    case FTW_DP:
				                    return debug_verify(::rmdir(filepath) != -1, "failed with errno ", errno) ? 0 : -1;
			                    case FTW_DNR:
			                    case FTW_NS:
				                    return debug_fail("unknown file") ? 0 : -1;
			                    default:
				                    debug_unreachable("unknown file type");
			                    }
		                    },
		                    7, // arbitrary
		                    FTW_DEPTH | FTW_MOUNT | FTW_PHYS) != -1, "failed to remove temporary directory \"", filepath, "\"");
	}

	void remove_directory(ext::index index)
	{
		debug_assert(directory_metas[index].alias_count <= 0);

		if (directory_metas[index].temporary)
		{
			purge_temporary_directory(directory_metas[index].filepath.data());
		}

		directory_filepath_assets.erase(directory_filepath_assets.begin() + index);
		directory_inotify_fds.erase(directory_inotify_fds.begin() + index);
		directory_metas.erase(directory_metas.begin() + index);
	}

	bool try_read(utility::heap_string_utf8 && filepath, watch_callback & watch_callback, engine::Asset match)
	{
		const int fd = ::open(filepath.data(), O_RDONLY);
		if (!debug_verify(fd != -1, "open \"", filepath, "\" failed with errno ", errno))
			return false;

		core::ReadStream stream([](void * dest, ext::usize n, void * data)
		                        {
			                        const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

			                        return ext::read_some_nonzero(fd, dest, n);
		                        },
		                        reinterpret_cast<void *>(fd),
		                        std::move(filepath));

		watch_callback.callback(std::move(stream), watch_callback.data, match);

		debug_verify(::close(fd) != -1, "failed with errno ", errno);
		return true;
	}

	template <typename F>
	ext::ssize scan_directory(const directory_meta & directory_meta, F && f, ext::usize max = ext::usize(-1))
	{
		if (!debug_assert(max != 0))
			return max;

		DIR * const dir = ::opendir(directory_meta.filepath.data());
		if (!debug_verify(dir != nullptr, "opendir failed with errno ", errno))
			return -1;

		ext::ssize number_of_matches = 0;

		while (struct dirent * const entry = ::readdir(dir))
		{
			const auto filename = utility::string_view_utf8(entry->d_name);

			const engine::Asset full_asset(filename.data(), filename.size());
			const auto maybe_full = std::find(directory_meta.fulls.assets.begin(), directory_meta.fulls.assets.end(), full_asset);
			if (maybe_full != directory_meta.fulls.assets.end())
			{
				if (f(filename, directory_meta.fulls, maybe_full - directory_meta.fulls.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
						break;
				}

				continue;
			}

			const auto dot = utility::unit_difference(filename.rfind('.')).get();
			if (dot == ext::index_invalid)
				return false; // not eligible for partial matching

			const engine::Asset name_asset(filename.data(), dot);
			const auto maybe_name = std::find(directory_meta.names.assets.begin(), directory_meta.names.assets.end(), name_asset);
			if (maybe_name != directory_meta.names.assets.end())
			{
				if (f(filename, directory_meta.names, maybe_name - directory_meta.names.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
						break;
				}

				continue;
			}

			const engine::Asset extension_asset(filename.data() + dot, filename.size() - dot);
			const auto maybe_extension = std::find(directory_meta.extensions.assets.begin(), directory_meta.extensions.assets.end(), extension_asset);
			if (maybe_extension != directory_meta.extensions.assets.end())
			{
				if (f(filename, directory_meta.extensions, maybe_extension - directory_meta.extensions.assets.begin()))
				{
					if (ext::usize(++number_of_matches) >= max)
						break;
				}

				continue;
			}
		}

		debug_verify(::closedir(dir) != -1, "failed with errno ", errno);
		return number_of_matches;
	}

	bool try_start_watch(int notify_fd, ext::index directory_index)
	{
		debug_assert(directory_metas[directory_index].alias_count > 0);
		debug_assert(directory_inotify_fds[directory_index] == -1);

		uint32_t mask = IN_CLOSE_WRITE | IN_DELETE;
#if MODE_DEBUG
		mask |= IN_ATTRIB | IN_MODIFY | IN_MOVE | IN_CREATE;
#endif
		debug_printline("starting watch of \"", directory_metas[directory_index].filepath, "\"");
		const int fd = ::inotify_add_watch(notify_fd, directory_metas[directory_index].filepath.data(), mask);
		directory_inotify_fds[directory_index] = fd;
		return debug_verify(fd != -1, "inotify_add_watch failed with errno ", errno);
	}

	void stop_watch(int notify_fd, ext::index directory_index)
	{
		if (directory_inotify_fds[directory_index] != -1)
		{
			debug_printline("stopping watch of \"", directory_metas[directory_index].filepath, "\"");
			debug_verify(::inotify_rm_watch(notify_fd, directory_inotify_fds[directory_index]) != -1, "failed with errno ", errno);
			directory_inotify_fds[directory_index] = -1;
		}
	}

	void clear_directories(int inotify_fd)
	{
		for (auto index : ranges::index_sequence_for(directory_metas))
		{
			stop_watch(inotify_fd, index);

			if (directory_metas[index].temporary)
			{
				purge_temporary_directory(directory_metas[index].filepath.data());
			}
		}

		directory_filepath_assets.clear();
		directory_inotify_fds.clear();
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

	void bind_alias_to_directory(engine::Asset alias, ext::index directory_index, int notify_fd)
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
				stop_watch(notify_fd, previous_directory_index);
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

	int message_pipe[2];
	core::async::Thread thread;

	void file_watch()
	{
		const int notify_fd = inotify_init1(IN_NONBLOCK);
		if (!debug_verify(notify_fd >= 0, "inotify failed with ", errno))
			return;

		auto panic =
			[notify_fd]()
			{
				clear_aliases();
				clear_directories(notify_fd);
				clear_watches();
				::close(notify_fd);
			};

		struct pollfd fds[2] = {
			{message_pipe[0], POLLIN, 0},
			{notify_fd, POLLIN, 0},
		};

		while (true)
		{
			const int n = poll(fds, sizeof fds / sizeof fds[0], -1);
			if (n < 0)
			{
				if (debug_verify(errno == EINTR))
					continue;

				return panic();
			}

			debug_assert(0 < n, "unexpected timeout");

			if (!debug_verify((fds[0].revents & ~fds[0].events) == 0))
				return panic();

			if (fds[0].revents & POLLIN)
			{
				Message message;
				while (message_queue.try_pop(message))
				{
					char buffer[1];
					debug_verify(ext::read_all_nonzero(message_pipe[0], buffer, sizeof buffer) == sizeof buffer);

					if (utility::holds_alternative<Terminate>(message))
					{
						debug_assert(alias_assets.empty());
						debug_assert(directory_filepath_assets.empty());
						debug_assert(watch_ids.empty());
						return panic(); // can we really call it panic if it is intended?
					}

					struct
					{
						int notify_fd;

						void operator () (RegisterDirectory && x)
						{
							if (x.filepath.back() != '/')
							{
								if (!debug_verify(x.filepath.try_append('/')))
									return; // error
							}

							const auto directory_index = find_or_add_directory(std::move(x.filepath));
							if (!debug_verify(directory_index != ext::index_invalid))
								return; // error

							bind_alias_to_directory(x.alias, directory_index, notify_fd);
						}

						void operator () (RegisterTemporaryDirectory && x)
						{
							utility::heap_string_utf8 filepath(u8"" P_tmpdir "/unnamed-XXXXXX"); // todo project name
							if (!debug_verify(::mkdtemp(filepath.data()) != nullptr, "mkdtemp failed with errno ", errno))
								return; // error

							if (!debug_verify(filepath.try_append('/')))
							{
								purge_temporary_directory(filepath.data());
								return; // error
							}

							debug_printline("created temporary directory \"", filepath, "\"");

							const auto directory_index = find_or_add_directory(std::move(filepath), temporary_directory{true});
							if (!debug_verify(directory_index != ext::index_invalid, "mkdtemp lied to us!"))
							{
								purge_temporary_directory(filepath.data());
								return; // error
							}

							bind_alias_to_directory(x.alias, directory_index, notify_fd);
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
								stop_watch(notify_fd, directory_index);
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

							directory_meta temporary_meta(utility::heap_string_utf8(directory_metas[directory_index].filepath), false);

							debug_expression(bool added_any_match = false);
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

										temporary_meta.extensions.assets.push_back(asset);
										debug_expression(added_any_match = true);
									}
									else if (x.pattern.data()[found.get() - 1] == '*') // name
									{
										const utility::string_view_utf8 name(x.pattern.data() + from.get(), found - from - 1); // ingnore '*'
										const engine::Asset asset(name.data(), name.size());

										temporary_meta.names.assets.push_back(asset);
										debug_expression(added_any_match = true);
									}
									else // full
									{
										const utility::string_view_utf8 full(x.pattern.data() + from.get(), found - from);
										const engine::Asset asset(full.data(), full.size());

										temporary_meta.fulls.assets.push_back(asset);
										debug_expression(added_any_match = true);
									}
								}

								if (found == x.pattern.size())
									break; // done

								from = found + 1; // skip '|'
							}
							debug_assert(added_any_match, "nothing to read in directory, please sanitize your data!");

							watch_callback watch_callback{x.callback, std::move(x.data), engine::Asset{}, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING), static_cast<bool>(x.mode & engine::file::flags::ONCE_ONLY)};

							const ext::usize number_of_matches = scan_directory(
								temporary_meta,
								[&temporary_meta, &watch_callback](utility::string_view_utf8 filename, const struct directory_meta::match & match, ext::index match_index)
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

						void operator () (Remove && x)
						{
							const auto alias_index = find_alias(x.directory);
							if (!debug_verify(alias_index != ext::index_invalid))
								return; // error

							const auto directory_index = find_directory(alias_metas[alias_index].filepath_asset);
							if (!debug_assert(directory_index != ext::index_invalid))
								return; // error

							auto & directory_meta = directory_metas[directory_index];

							scan_directory(
								directory_meta,
								[&directory_meta](utility::string_view_utf8 filename, const struct directory_meta::match & /*match*/, ext::index /*match_index*/)
								{
									auto filepath = directory_meta.filepath + filename;
									debug_verify(::unlink(filepath.data()) != -1, "failed with errno ", errno);
									return true;
								});
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

							const auto watch_id = add_watch(x.callback, std::move(x.data), x.directory, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING), static_cast<bool>(x.mode & engine::file::flags::ONCE_ONLY));

							alias_metas[alias_index].watches.push_back(watch_id);

							debug_expression(bool added_any_match = false);
							auto from = utility::unit_difference(0);
							while (true)
							{
								auto found = utility::unit_difference(x.pattern.find('|', from));
								if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
								{
									if (x.pattern.data()[from.get()] == '*') // extension
									{
										const utility::string_view_utf8 extension(x.pattern.data() + from.get() + 1, found - from - 1); // ingnore '*'
										debug_printline("adding extension \"", extension, "\" to watch for \"", directory_meta.filepath, "\"");
										const engine::Asset asset(extension.data(), extension.size());

										directory_meta.extensions.assets.push_back(asset);
										directory_meta.extensions.aliases.push_back(x.directory);
										directory_meta.extensions.watches.push_back(watch_id);
										debug_expression(added_any_match = true);
									}
									else if (x.pattern.data()[found.get() - 1] == '*') // name
									{
										const utility::string_view_utf8 name(x.pattern.data() + from.get(), found - from - 1); // ingnore '*'
										debug_printline("adding name \"", name, "\" to watch for \"", directory_meta.filepath, "\"");
										const engine::Asset asset(name.data(), name.size());

										directory_meta.names.assets.push_back(asset);
										directory_meta.names.aliases.push_back(x.directory);
										directory_meta.names.watches.push_back(watch_id);
										debug_expression(added_any_match = true);
									}
									else // full
									{
										const utility::string_view_utf8 full(x.pattern.data() + from.get(), found - from);
										debug_printline("adding full \"", full, "\" to watch for \"", directory_meta.filepath, "\"");
										const engine::Asset asset(full.data(), full.size());

										directory_meta.fulls.assets.push_back(asset);
										directory_meta.fulls.aliases.push_back(x.directory);
										directory_meta.fulls.watches.push_back(watch_id);
										debug_expression(added_any_match = true);
									}
								}

								if (found == x.pattern.size())
									break; // done

								from = found + 1; // skip '|'
							}
							debug_assert(added_any_match, "nothing to watch in directory, please sanitize your data!");

							if (!(x.mode & engine::file::flags::IGNORE_EXISTING))
							{
								if (!debug_assert(watch_ids.back() == watch_id))
									return;

								auto & watch_callback = watch_callbacks.back();

								ext::usize number_of_matches = scan_directory(
									directory_meta,
									[&directory_meta, &watch_callback, watch_id](utility::string_view_utf8 filename, const struct directory_meta::match & match, ext::index match_index)
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

							if (directory_inotify_fds[directory_index] == -1)
							{
								// todo files that are created in between the
								// scan and the start of the watch will not be
								// reported, maybe watch before scan and
								// remove duplicates?
								try_start_watch(notify_fd, directory_index);
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

							auto filepath = directory_meta.filepath + x.filename;

							int flags = O_WRONLY | O_CREAT;
							if (x.mode & engine::file::flags::APPEND_EXISTING)
							{
								flags |= O_APPEND;
							}
							else if (!(x.mode & engine::file::flags::OVERWRITE_EXISTING))
							{
								flags |= O_EXCL;
							}
							const int fd = ::open(filepath.data(), flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
							if (fd == -1)
							{
								debug_verify(errno == EEXIST, "open \"", filepath, "\" failed with errno ", errno);
								return; // error
							}

							core::WriteStream stream([](const void * src, ext::usize n, void * data)
							                         {
								                         const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

								                         return ext::write_some_nonzero(fd, src, n);
							                         },
							                         reinterpret_cast<void *>(fd),
							                         std::move(filepath));
							x.callback(std::move(stream), std::move(x.data));

							debug_verify(::close(fd) != -1, "failed with errno ", errno);
						}

						void operator () (Terminate &&)
						{
							intrinsic_unreachable();
						}

					} visitor{notify_fd};

					visit(visitor, std::move(message));
				}
			}

			if (fds[1].revents & POLLIN)
			{
				std::aligned_storage_t<4096, alignof(struct inotify_event)> buffer; // arbitrary

				while (true)
				{
					const ext::ssize n = ::read(fds[1].fd, &buffer, sizeof buffer);
					debug_assert(n != 0, "unexpected eof");
					if (n < 0)
					{
						if (debug_verify(errno == EAGAIN))
							break;

						return panic();
					}

					const char * const begin = reinterpret_cast<const char *>(&buffer);
					const char * const end = begin + n;
					for (const char * ptr = begin; ptr != end;)
					{
						const struct inotify_event * const event = reinterpret_cast<const struct inotify_event *>(ptr);
						if (!debug_assert((sizeof(struct inotify_event) + event->len) % alignof(struct inotify_event) == 0))
							return panic();

						ptr += sizeof(struct inotify_event) + event->len;

#if MODE_DEBUG
						if (event->mask & IN_ATTRIB) { debug_printline("event IN_ATTRIB: ", event->name); }
						if (event->mask & IN_MODIFY) { debug_printline("event IN_MODIFY: ", event->name); }
						if (event->mask & IN_CLOSE_WRITE) { debug_printline("event IN_CLOSE_WRITE: ", event->name); }
						if (event->mask & IN_MOVED_FROM) { debug_printline("event IN_MOVED_FROM: ", event->name); }
						if (event->mask & IN_MOVED_TO) { debug_printline("event IN_MOVED_TO: ", event->name); }
						if (event->mask & IN_CREATE) { debug_printline("event IN_CREATE: ", event->name); }
						if (event->mask & IN_DELETE) { debug_printline("event IN_DELETE: ", event->name); }
#endif

						const auto directory_index = find_directory(event->wd);
						if (directory_index == ext::index_invalid)
							continue; // must have been removed :shrug:

						const auto & directory_meta = directory_metas[directory_index];

						const auto filename = utility::string_view_utf8(event->name);

						const engine::Asset full_asset(filename.data(), filename.size());
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

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory_meta.filepath + filename, watch_callback, full_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE && watch_callback.report_missing)
								{
									const auto number_of_matches = scan_directory(
										directory_meta,
										[watch_id](utility::string_view_utf8 /*filename*/, const struct directory_meta::match & match, ext::index match_index)
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
										stop_watch(fds[1].fd, directory_index);
									}
									remove_watch(watch_callback.alias, watch_id);
									remove_watch(watch_id);
								}
							}
							continue;
						}

						const auto dot = utility::unit_difference(filename.rfind('.')).get();
						if (dot == ext::index_invalid)
							continue; // not eligible for partial matching

						const engine::Asset name_asset(filename.data(), dot);
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

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory_meta.filepath + filename, watch_callback, name_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE)
								{
									const auto number_of_matches = scan_directory(
										directory_meta,
										[watch_id](utility::string_view_utf8 /*filename*/, const struct directory_meta::match & match, ext::index match_index)
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
										stop_watch(fds[1].fd, directory_index);
									}
									remove_watch(watch_callback.alias, watch_id);
									remove_watch(watch_id);
								}
							}
							continue;
						}

						const engine::Asset extension_asset(filename.data() + dot, filename.size() - dot);
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

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory_meta.filepath + filename, watch_callback, extension_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE)
								{
									const auto number_of_matches = scan_directory(
										directory_meta,
										[watch_id](utility::string_view_utf8 /*filename*/, const struct directory_meta::match & match, ext::index match_index)
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
										stop_watch(fds[1].fd, directory_index);
									}
									remove_watch(watch_callback.alias, watch_id);
									remove_watch(watch_id);
								}
							}
							continue;
						}
					}
				}
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
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}

			thread.join();

			::close(message_pipe[0]);
			::close(message_pipe[1]);

			// if (auto size = message_queue.clear())
			// {
			// 	debug_printline("dropped ", size, " messages on exit");
			// }
		}

		system::system()
		{
			if (!debug_verify(::pipe(message_pipe) != -1, "failed with errno ", errno))
				return;

			thread = core::async::Thread(file_watch);
			if (!debug_verify(thread.valid()))
			{
				::close(message_pipe[0]);
				::close(message_pipe[1]);
				return;
			}
		}

		void register_directory(engine::Asset name, utility::heap_string_utf8 && path)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<RegisterDirectory>, name, std::move(path))))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void register_temporary_directory(engine::Asset name)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<RegisterTemporaryDirectory>, name)))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void unregister_directory(engine::Asset name)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<UnregisterDirectory>, name)))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void read(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Read>, directory, std::move(pattern), callback, std::move(data), mode)))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void remove(engine::Asset directory, utility::heap_string_utf8 && pattern)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Remove>, directory, std::move(pattern))))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void watch(
			engine::Asset directory,
			utility::heap_string_utf8 && pattern,
			watch_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Watch>, directory, std::move(pattern), callback, std::move(data), mode)))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}

		void write(
			engine::Asset directory,
			utility::heap_string_utf8 && filename,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (!debug_assert(thread.valid()))
				return;

			if (debug_verify(message_queue.try_emplace(utility::in_place_type<Write>, directory, std::move(filename), callback, std::move(data), mode)))
			{
				const char zero = 0;
				debug_verify(::write(message_pipe[1], &zero, sizeof zero) == sizeof zero);
			}
		}
	}
}

#endif
