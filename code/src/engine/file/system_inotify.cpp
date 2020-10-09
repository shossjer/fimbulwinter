#include "config.h"

#if FILE_USE_INOTIFY

#include "system.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/ReadStream.hpp"
#include "core/WriteStream.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/algorithm/remove.hpp"
#include "utility/alias.hpp"
#include "utility/any.hpp"
#include "utility/container/vector.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/ext/unistd.hpp"
#include "utility/functional/find.hpp"
#include "utility/functional/utility.hpp"
#include "utility/preprocessor/common.hpp"
#include "utility/preprocessor/count_arguments.hpp"
#include "utility/preprocessor/for_each.hpp"
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

	class watch_vector
	{
		utility::heap_vector<watch_id, watch_callback> data_;

	public:
		using iterator = utility::heap_vector<watch_id, watch_callback>::iterator;
		using const_iterator = utility::heap_vector<watch_id, watch_callback>::const_iterator;

		iterator end() { return data_.end(); }
		const_iterator end() const { return data_.end(); }

		bool empty() const { return ext::empty(data_); }

		iterator find_by_id(watch_id id) { return ext::find_if(data_, fun::first == id); }
		const_iterator find_by_id(watch_id id) const { return ext::find_if(data_, fun::first == id); }

		iterator add(
			engine::file::watch_callback * callback,
			utility::any && data,
			engine::Asset alias,
			bool report_missing,
			bool once_only)
		{
			static watch_id next_id = 1; // reserve 0 (for no particular reason)
			const auto watch_id = next_id++;

			if (!debug_verify(data_.try_emplace_back(
				                  std::piecewise_construct,
				                  std::forward_as_tuple(watch_id),
				                  std::forward_as_tuple(callback, std::move(data), alias, report_missing, once_only))))
				return data_.end();
			else
				return --data_.end();
		}

		void remove(iterator it) { data_.erase(it); }

		void remove_by_watches(const utility::heap_vector<watch_id> & watches) // todo span/range
		{
			data_.erase(
				ext::remove_if(data_, fun::first | fun::contains(watches, fun::_1)),
				data_.end());
		}

		void clear() { data_.clear(); }
	} watches;

	define_alias(as_watch, id, callback);

	class match_vector
	{
		utility::heap_vector<engine::Asset, engine::Asset, watch_id> data_;

	public:
		using iterator = utility::heap_vector<engine::Asset, engine::Asset, watch_id>::iterator;
		using const_iterator = utility::heap_vector<engine::Asset, engine::Asset, watch_id>::const_iterator;

		iterator end() { return data_.end(); }
		const_iterator end() const { return data_.end(); }

		bool empty() const { return ext::empty(data_); }

		iterator find_by_asset(engine::Asset asset) { return ext::find_if(data_, fun::get<0> == asset); }
		iterator find_by_alias(engine::Asset alias) { return ext::find_if(data_, fun::get<1> == alias); }
		iterator find_by_watch(watch_id watch) { return ext::find_if(data_, fun::get<2> == watch); }
		const_iterator find_by_asset(engine::Asset asset) const { return ext::find_if(data_, fun::get<0> == asset); }
		const_iterator find_by_alias(engine::Asset alias) const { return ext::find_if(data_, fun::get<1> == alias); }
		const_iterator find_by_watch(watch_id watch) const { return ext::find_if(data_, fun::get<2> == watch); }

		bool add_match(engine::Asset asset, engine::Asset alias, watch_id watch) // todo return iterator
		{
			return debug_verify(data_.try_emplace_back(asset, alias, watch));
		}

		void remove(iterator it) { data_.erase(it); }

		void remove_by_watch(watch_id watch)
		{
			data_.erase(ext::remove_if(data_, fun::get<2> == watch), data_.end());
		}

		void remove_copy_by_alias(engine::Asset alias, match_vector & to)
		{
			data_.erase(
				ext::remove_copy_if(data_, ext::back_inserter(to.data_), fun::get<1> == alias),
				data_.end());
		}

		void clear() { data_.clear(); }
	};

	define_alias(as_match, asset, alias, watch);

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

	struct directory_meta
	{
		utility::heap_string_utf8 filepath;
		bool temporary;
		int alias_count;

		match_vector fulls;
		match_vector names;
		match_vector extensions;

		directory_meta(utility::heap_string_utf8 && filepath, bool temporary)
			: filepath(std::move(filepath))
			, temporary(temporary)
			, alias_count(0)
		{}

		void remove_by_watch(watch_id watch)
		{
			fulls.remove_by_watch(watch);
			names.remove_by_watch(watch);
			extensions.remove_by_watch(watch);
		}

		void remove_copy_by_alias(engine::Asset alias, directory_meta & to)
		{
			fulls.remove_copy_by_alias(alias, to.fulls);
			names.remove_copy_by_alias(alias, to.names);
			extensions.remove_copy_by_alias(alias, to.extensions);
		}
	};

	struct temporary_directory { bool flag; /*explicit temporary_directory(bool flag) : flag(flag) {}*/ };

	define_alias(as_directory, asset, fd, meta);

	class directory_vector
	{
		utility::heap_vector<engine::Asset, int, directory_meta> data_;

	public:
		using iterator = utility::heap_vector<engine::Asset, int, directory_meta>::iterator;
		using const_iterator = utility::heap_vector<engine::Asset, int, directory_meta>::const_iterator;
		using reference = utility::heap_vector<engine::Asset, int, directory_meta>::reference;
		using const_reference = utility::heap_vector<engine::Asset, int, directory_meta>::const_reference;

		iterator begin() { return data_.begin(); }
		const_iterator begin() const { return data_.begin(); }
		iterator end() { return data_.end(); }
		const_iterator end() const { return data_.end(); }

		bool empty() const { return ext::empty(data_); }

		iterator find_by_asset(engine::Asset filepath_asset) { return ext::find_if(data_, fun::get<0> == filepath_asset); }
		iterator find_by_fd(int fd) { return ext::find_if(data_, fun::get<1> == fd); }
		const_iterator find_by_asset(engine::Asset filepath_asset) const { return ext::find_if(data_, fun::get<0> == filepath_asset); }
		const_iterator find_by_fd(int fd) const { return ext::find_if(data_, fun::get<1> == fd); }

		iterator find_or_add_directory(utility::heap_string_utf8 && filepath, temporary_directory is_temporary = {false})
		{
			const engine::Asset asset(filepath.data(), filepath.size());

			const auto directory_it = find_by_asset(asset);
			if (directory_it != data_.end())
			{
				debug_assert(as_directory(*directory_it).meta.temporary == is_temporary.flag);
				return directory_it;
			}

			struct stat64 buf;
			if (!debug_verify(::stat64(filepath.data(), &buf) != -1, "failed with errno ", errno))
				return data_.end();

			if (!debug_verify(S_ISDIR(buf.st_mode), "trying to register a nondirectory"))
				return data_.end();

			if (!debug_verify(data_.try_emplace_back(
				                  std::piecewise_construct,
				                  std::forward_as_tuple(asset),
				                  std::forward_as_tuple(-1),
				                  std::forward_as_tuple(std::move(filepath), is_temporary.flag))))
				return data_.end();
			else
				return --data_.end();
		}

		iterator erase(iterator it)
		{
			auto && directory = as_directory(*it);

			debug_assert(directory.meta.alias_count <= 0);

			if (directory.meta.temporary)
			{
				purge_temporary_directory(directory.meta.filepath.data());
			}

			return data_.erase(it);
		}

		void clear() { data_.clear(); }
	} directories;

	using directory_reference = decltype(as_directory(std::declval<directory_vector::reference>()));

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
			const auto filename = utility::string_units_utf8(entry->d_name);

			const engine::Asset full_asset(filename.data(), filename.size());
			const auto full_it = directory_meta.fulls.find_by_asset(full_asset);
			if (full_it != directory_meta.fulls.end())
			{
				if (f(filename, as_match(*full_it)))
				{
					if (ext::usize(++number_of_matches) >= max)
						break;
				}

				continue;
			}

			const auto dot = rfind(filename, '.');
			if (dot == filename.end())
				continue; // not eligible for partial matching

			const engine::Asset name_asset(utility::string_units_utf8(filename.begin(), dot));
			const auto name_it = directory_meta.names.find_by_asset(name_asset);
			if (name_it != directory_meta.names.end())
			{
				if (f(filename, as_match(*name_it)))
				{
					if (ext::usize(++number_of_matches) >= max)
						break;
				}

				continue;
			}

			const engine::Asset extension_asset(utility::string_units_utf8(dot, filename.end()));
			const auto extension_it = directory_meta.extensions.find_by_asset(extension_asset);
			if (extension_it != directory_meta.extensions.end())
			{
				if (f(filename, as_match(*extension_it)))
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

	bool try_start_watch(int notify_fd, directory_reference directory)
	{
		debug_assert(0 < directory.meta.alias_count);
		debug_assert(directory.fd == -1);

		uint32_t mask = IN_CLOSE_WRITE | IN_DELETE;
#if MODE_DEBUG
		mask |= IN_ATTRIB | IN_MODIFY | IN_MOVE | IN_CREATE;
#endif
		debug_printline("starting watch of \"", directory.meta.filepath, "\"");
		const int fd = ::inotify_add_watch(notify_fd, directory.meta.filepath.data(), mask);
		directory.fd = fd;
		return debug_verify(fd != -1, "inotify_add_watch failed with errno ", errno);
	}

	void stop_watch(int notify_fd, directory_reference directory)
	{
		if (directory.fd != -1)
		{
			debug_printline("stopping watch of \"", directory.meta.filepath, "\"");
			debug_verify(::inotify_rm_watch(notify_fd, directory.fd) != -1, "failed with errno ", errno);
			directory.fd = -1;
		}
	}

	void clear_directories(int inotify_fd)
	{
		for (auto && d : directories)
		{
			auto && directory = as_directory(d);

			stop_watch(inotify_fd, directory);

			if (directory.meta.temporary)
			{
				purge_temporary_directory(directory.meta.filepath.data());
			}
		}

		directories.clear();
	}

	struct alias_meta
	{
		engine::Asset filepath_asset;

		utility::heap_vector<watch_id> watches;

		alias_meta(engine::Asset filepath_asset)
			: filepath_asset(filepath_asset)
		{}
	};

	class alias_vector
	{
		utility::heap_vector<engine::Asset, alias_meta> data_;

	public:
		using iterator = utility::heap_vector<engine::Asset, alias_meta>::iterator;
		using const_iterator = utility::heap_vector<engine::Asset, alias_meta>::const_iterator;

		iterator end() { return data_.end(); }
		const_iterator end() const { return data_.end(); }

		bool empty() const { return ext::empty(data_); }

		iterator find_by_asset(engine::Asset asset) { return ext::find_if(data_, fun::first == asset); }
		const_iterator find_by_asset(engine::Asset asset) const { return ext::find_if(data_, fun::first == asset); }

		iterator add(engine::Asset asset, engine::Asset filepath)
		{
			if (!debug_verify(data_.try_emplace_back(asset, filepath)))
				return data_.end();

			return --data_.end();
		}

		void remove(iterator alias_it)
		{
			watches.remove_by_watches(alias_it.second->watches);

			data_.erase(alias_it);
		}

		void clear() { data_.clear(); }
	} aliases;

	define_alias(as_alias, asset, meta);

	void bind_alias_to_directory(engine::Asset alias_asset, directory_reference directory, int notify_fd)
	{
		const auto alias_it = aliases.find_by_asset(alias_asset);
		if (alias_it == aliases.end())
		{
			aliases.add(alias_asset, directory.asset);
			directory.meta.alias_count++;
		}
		else
		{
			auto && alias = as_alias(*alias_it);

			const auto previous_directory_it = directories.find_by_asset(alias.meta.filepath_asset);
			debug_assert(previous_directory_it != directories.end());

			auto && previous_directory = as_directory(*previous_directory_it);

			alias.meta.filepath_asset = directory.asset;
			directory.meta.alias_count++;

			previous_directory.meta.remove_copy_by_alias(alias.asset, directory.meta);

			if (--previous_directory.meta.alias_count <= 0)
			{
				stop_watch(notify_fd, previous_directory);
				directories.erase(previous_directory_it);
			}
		}
	}

	void remove_watch(engine::Asset alias_asset, watch_id watch)
	{
		const auto alias_it = aliases.find_by_asset(alias_asset);
		if (!debug_assert(alias_it != aliases.end()))
			return;

		auto && alias = as_alias(*alias_it);

		auto watch_it = ext::find(alias.meta.watches, watch);
		if (!debug_assert(watch_it != alias.meta.watches.end()))
			return;

		watch_it = alias.meta.watches.erase(watch_it);

		debug_assert(ext::find(watch_it, alias.meta.watches.end(), watch) == alias.meta.watches.end());
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
				aliases.clear();
				clear_directories(notify_fd);
				watches.clear();
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
						debug_assert(aliases.empty());
						debug_assert(directories.empty());
						debug_assert(watches.empty());
						return panic(); // can we really call it panic if it is intended?
					}

					struct
					{
						int notify_fd;

						void operator () (RegisterDirectory && x)
						{
							if (!debug_assert(!empty(x.filepath)))
								return;

							if (back(x.filepath) != '/')
							{
								if (!debug_verify(x.filepath.try_append('/')))
									return; // error
							}

							const auto directory_it = directories.find_or_add_directory(std::move(x.filepath));
							if (!debug_verify(directory_it != directories.end()))
								return; // error

							bind_alias_to_directory(x.alias, as_directory(*directory_it), notify_fd);
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

							const auto directory_it = directories.find_or_add_directory(std::move(filepath), temporary_directory{true});
							if (!debug_verify(directory_it != directories.end(), "mkdtemp lied to us!"))
							{
								purge_temporary_directory(filepath.data());
								return; // error
							}

							bind_alias_to_directory(x.alias, as_directory(*directory_it), notify_fd);
						}

						void operator () (UnregisterDirectory && x)
						{
							const auto alias_it = aliases.find_by_asset(x.alias);
							if (!debug_verify(alias_it != aliases.end(), "directory alias does not exist"))
								return; // error

							auto && alias = as_alias(*alias_it);

							const auto directory_it = directories.find_by_asset(alias.meta.filepath_asset);
							if (!debug_assert(directory_it != directories.end()))
								return; // error

							auto && directory = as_directory(*directory_it);

							aliases.remove(alias_it);

							if (--directory.meta.alias_count <= 0)
							{
								stop_watch(notify_fd, directory);
								directories.erase(directory_it);
							}
						}

						void operator () (Read && x)
						{
							const auto alias_it = aliases.find_by_asset(x.directory);
							if (!debug_verify(alias_it != aliases.end()))
								return; // error

							auto && alias = as_alias(*alias_it);

							const auto directory_it = directories.find_by_asset(alias.meta.filepath_asset);
							if (!debug_assert(directory_it != directories.end()))
								return;

							auto && directory = as_directory(*directory_it);

							directory_meta temporary_meta(utility::heap_string_utf8(directory.meta.filepath), false);

							debug_expression(bool added_any_match = false);
							auto from = x.pattern.begin();
							while (true)
							{
								auto found = find(from, x.pattern.end(), '|');
								if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
								{
									if (*from == '*') // extension
									{
										const utility::string_units_utf8 extension(from + 1, found); // ingnore '*'
										const engine::Asset asset(extension.data(), extension.size());

										debug_verify(temporary_meta.extensions.add_match(asset, engine::Asset{}, watch_id{}));
										debug_expression(added_any_match = true);
									}
									else if (*(found - 1) == '*') // name
									{
										const utility::string_units_utf8 name(from, found - 1); // ingnore '*'
										const engine::Asset asset(name.data(), name.size());

										debug_verify(temporary_meta.names.add_match(asset, engine::Asset{}, watch_id{}));
										debug_expression(added_any_match = true);
									}
									else // full
									{
										const utility::string_units_utf8 full(from, found);
										const engine::Asset asset(full.data(), full.size());

										debug_verify(temporary_meta.fulls.add_match(asset, engine::Asset{}, watch_id{}));
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
								[&temporary_meta, &watch_callback](utility::string_units_utf8 filename, auto && match)
								{
									try_read(temporary_meta.filepath + filename, watch_callback, match.asset);
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
							const auto alias_it = aliases.find_by_asset(x.directory);
							if (!debug_verify(alias_it != aliases.end()))
								return; // error

							auto && alias = as_alias(*alias_it);

							const auto directory_it = directories.find_by_asset(alias.meta.filepath_asset);
							if (!debug_assert(directory_it != directories.end()))
								return; // error

							auto && directory = as_directory(*directory_it);

							scan_directory(
								directory.meta,
								[&directory](utility::string_units_utf8 filename, auto && /*match*/)
								{
									auto filepath = directory.meta.filepath + filename;
									debug_verify(::unlink(filepath.data()) != -1, "failed with errno ", errno);
									return true;
								});
						}

						void operator () (Watch && x)
						{
							const auto alias_it = aliases.find_by_asset(x.directory);
							if (!debug_verify(alias_it != aliases.end()))
								return; // error

							auto && alias = as_alias(*alias_it);

							const auto directory_it = directories.find_by_asset(alias.meta.filepath_asset);
							if (!debug_assert(directory_it != directories.end()))
								return; // error

							auto && directory = as_directory(*directory_it);

							const auto watch_it = watches.add(x.callback, std::move(x.data), x.directory, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING), static_cast<bool>(x.mode & engine::file::flags::ONCE_ONLY));
							if (!debug_assert(watch_it != watches.end()))
								return; // error

							auto && watch = as_watch(*watch_it);

							debug_verify(alias.meta.watches.push_back(watch.id));

							debug_expression(bool added_any_match = false);
							auto from = x.pattern.begin();
							while (true)
							{
								auto found = find(from, x.pattern.end(), '|');
								if (debug_assert(found != from, "found empty pattern, please sanitize your data!"))
								{
									if (*from == '*') // extension
									{
										const utility::string_units_utf8 extension(from + 1, found); // ingnore '*'
										debug_printline("adding extension \"", extension, "\" to watch for \"", directory.meta.filepath, "\"");
										const engine::Asset asset(extension.data(), extension.size());

										debug_verify(directory.meta.extensions.add_match(asset, x.directory, watch.id));
										debug_expression(added_any_match = true);
									}
									else if (*(found - 1) == '*') // name
									{
										const utility::string_units_utf8 name(from, found - 1); // ingnore '*'
										debug_printline("adding name \"", name, "\" to watch for \"", directory.meta.filepath, "\"");
										const engine::Asset asset(name.data(), name.size());

										debug_verify(directory.meta.names.add_match(asset, x.directory, watch.id));
										debug_expression(added_any_match = true);
									}
									else // full
									{
										const utility::string_units_utf8 full(from, found);
										debug_printline("adding full \"", full, "\" to watch for \"", directory.meta.filepath, "\"");
										const engine::Asset asset(full.data(), full.size());

										debug_verify(directory.meta.fulls.add_match(asset, x.directory, watch.id));
										debug_expression(added_any_match = true);
									}
								}

								if (found == x.pattern.end())
									break; // done

								from = found + 1; // skip '|'
							}
							debug_assert(added_any_match, "nothing to watch in directory, please sanitize your data!");

							if (!(x.mode & engine::file::flags::IGNORE_EXISTING))
							{
								ext::usize number_of_matches = scan_directory(
									directory.meta,
									[&directory, &watch](utility::string_units_utf8 filename, auto && match)
									{
										if (match.watch != watch.id) // todo always newly added
											return false;

										try_read(directory.meta.filepath + filename, watch.callback, match.asset);
										return true;
									},
									watch.callback.once_only ? 1 : -1);
								if (watch.callback.report_missing && number_of_matches == 0)
								{
									number_of_matches++;
									watch.callback.callback(core::ReadStream(nullptr, nullptr, ""), watch.callback.data, engine::Asset(""));
								}

								if (watch.callback.once_only && number_of_matches >= 1)
								{
									directory.meta.remove_by_watch(watch.id); // todo always the last entries
									if (debug_assert(ext::back(alias.meta.watches) == watch.id))
									{
										ext::pop_back(alias.meta.watches);
									}
									watches.remove(watch_it); // always last
									return;
								}
							}

							if (directory.fd == -1)
							{
								// todo files that are created in between the
								// scan and the start of the watch will not be
								// reported, maybe watch before scan and
								// remove duplicates?
								try_start_watch(notify_fd, directory);
							}
						}

						void operator () (Write && x)
						{
							const auto alias_it = aliases.find_by_asset(x.directory);
							if (!debug_verify(alias_it != aliases.end()))
								return; // error

							auto && alias = as_alias(*alias_it);

							const auto directory_it = directories.find_by_asset(alias.meta.filepath_asset);
							if (!debug_assert(directory_it != directories.end()))
								return; // error

							const auto && directory = as_directory(*directory_it);

							auto filepath = directory.meta.filepath + x.filename;

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

						const auto directory_it = directories.find_by_fd(event->wd);
						if (directory_it == directories.end())
							continue; // must have been removed :shrug:

						auto && directory = as_directory(*directory_it);

						const auto filename = utility::string_units_utf8(event->name);

						const engine::Asset full_asset(filename.data(), filename.size());
						const auto full_it = directory.meta.fulls.find_by_asset(full_asset);
						if (full_it != directory.meta.fulls.end())
						{
							auto && full = as_match(*full_it);

							const auto watch_it = watches.find_by_id(full.watch);
							if (debug_assert(watch_it != watches.end()))
							{
								auto && watch = as_watch(*watch_it);

								bool handled = false;

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory.meta.filepath + filename, watch.callback, full_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE && watch.callback.report_missing)
								{
									const auto number_of_matches = scan_directory(
										directory.meta,
										[&watch](utility::string_units_utf8 /*filename*/, auto && match)
										{
											return match.watch == watch.id;
										});
									if (number_of_matches == 0)
									{
										watch.callback.callback(core::ReadStream(nullptr, nullptr, ""), watch.callback.data, engine::Asset(""));
										handled = true;
									}
								}

								if (handled && watch.callback.once_only)
								{
									directory.meta.remove_by_watch(watch.id);
									if (directory.meta.fulls.empty() &&
									    directory.meta.names.empty() &&
									    directory.meta.extensions.empty())
									{
										stop_watch(fds[1].fd, directory);
									}
									remove_watch(watch.callback.alias, watch.id);
									watches.remove(watch_it);
								}
							}
							continue;
						}

						const auto dot = rfind(filename, '.');
						if (dot == filename.end())
							continue; // not eligible for partial matching

						const engine::Asset name_asset(utility::string_units_utf8(filename.begin(), dot));
						const auto name_it = directory.meta.names.find_by_asset(name_asset);
						if (name_it != directory.meta.names.end())
						{
							auto && name = as_match(*name_it);

							const auto watch_it = watches.find_by_id(name.watch);
							if (debug_assert(watch_it != watches.end()))
							{
								auto && watch = as_watch(*watch_it);

								bool handled = false;

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory.meta.filepath + filename, watch.callback, name_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE)
								{
									const auto number_of_matches = scan_directory(
										directory.meta,
										[&watch](utility::string_units_utf8 /*filename*/, auto && match)
										{
											return match.watch == watch.id;
										});
									if (number_of_matches == 0)
									{
										watch.callback.callback(core::ReadStream(nullptr, nullptr, ""), watch.callback.data, engine::Asset(""));
										handled = true;
									}
								}

								if (handled && watch.callback.once_only)
								{
									directory.meta.remove_by_watch(watch.id);
									if (directory.meta.fulls.empty() &&
									    directory.meta.names.empty() &&
									    directory.meta.extensions.empty())
									{
										stop_watch(fds[1].fd, directory);
									}
									remove_watch(watch.callback.alias, watch.id);
									watches.remove(watch_it);
								}
							}
							continue;
						}

						const engine::Asset extension_asset(utility::string_units_utf8(dot, filename.end()));
						const auto extension_it = directory.meta.extensions.find_by_asset(extension_asset);
						if (extension_it != directory.meta.extensions.end())
						{
							auto && extension = as_match(*extension_it);

							const auto watch_it = watches.find_by_id(extension.watch);
							if (debug_assert(watch_it != watches.end()))
							{
								auto && watch = as_watch(*watch_it);

								bool handled = false;

								if (event->mask & IN_CLOSE_WRITE)
								{
									try_read(directory.meta.filepath + filename, watch.callback, extension_asset);
									handled = true;
								}

								if (event->mask & IN_DELETE)
								{
									const auto number_of_matches = scan_directory(
										directory.meta,
										[&watch](utility::string_units_utf8 /*filename*/, auto && match)
										{
											return match.watch == watch.id;
										});
									if (number_of_matches == 0)
									{
										watch.callback.callback(core::ReadStream(nullptr, nullptr, ""), watch.callback.data, engine::Asset(""));
										handled = true;
									}
								}

								if (handled && watch.callback.once_only)
								{
									directory.meta.remove_by_watch(watch.id);
									if (directory.meta.fulls.empty() &&
									    directory.meta.names.empty() &&
									    directory.meta.extensions.empty())
									{
										stop_watch(fds[1].fd, directory);
									}
									remove_watch(watch.callback.alias, watch.id);
									watches.remove(watch_it);
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
