#include "config.h"

#if FILE_WATCH_USE_INOTIFY

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"

#include "core/debug.hpp"
#include "engine/Asset.hpp"
#include "engine/file/system/works.hpp"
#include "engine/file/watch/watch.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/algorithm/find.hpp"
#include "utility/container/vector.hpp"
#include "utility/ext/unistd.hpp"
#include "utility/functional/utility.hpp"
#include "utility/shared_ptr.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>

namespace
{
	using fd_t = int;

	core::async::Thread watch_thread;
	int message_pipe[2];

	struct Message
	{
		enum Type : std::int16_t
		{
			ADD_READ,
			ADD_SCAN,
			REMOVE,
		};
		enum Flags : std::int16_t
		{
			NONE,
			RECURSE_DIRECTORIES,
			REPORT_MISSING,
		};

		Type type;
		Flags flags;
		engine::Identity id;
		union
		{
			ext::detail::shared_data<engine::file::ReadData> * read_ptr;
			ext::detail::shared_data<engine::file::ScanData> * scan_ptr;
		};

		Message() = default;

		explicit Message(Type type, Flags flags, engine::Identity id)
			: type(type)
			, flags(flags)
			, id(id)
		{}

		explicit Message(Type type, Flags flags, engine::Identity id, ext::detail::shared_data<engine::file::ReadData> * read_ptr)
			: type(type)
			, flags(flags)
			, id(id)
			, read_ptr(read_ptr)
		{}

		explicit Message(Type type, Flags flags, engine::Identity id, ext::detail::shared_data<engine::file::ScanData> * scan_ptr)
			: type(type)
			, flags(flags)
			, id(id)
			, scan_ptr(scan_ptr)
		{}
	};
	static_assert(std::is_trivially_copyable<Message>::value, "");

	struct ReadWatch
	{
		ext::heap_shared_ptr<engine::file::ReadData> ptr;
	};

	struct ScanWatch
	{
		ext::heap_shared_ptr<engine::file::ScanData> ptr;
	};

	struct ScanRecursiveWatch
	{
		ext::heap_shared_ptr<engine::file::ScanData> ptr;
	};

	core::container::Collection
	<
		engine::Identity,
		utility::heap_storage_traits,
		utility::heap_storage<ReadWatch>,
		utility::heap_storage<ScanWatch>,
		utility::heap_storage<ScanRecursiveWatch>
	>
	watches;

	struct Directory
	{
		utility::heap_vector<utility::string_units_utf8, ext::heap_shared_ptr<engine::file::ReadData>> reads;
		utility::heap_vector<utility::string_units_utf8, ext::heap_shared_ptr<engine::file::ReadData>> missing_reads;
		utility::heap_vector<ext::heap_shared_ptr<engine::file::ScanData>> scans;
		utility::heap_vector<ext::heap_shared_ptr<engine::file::ScanData>> recursive_scans;

		utility::heap_string_utf8 filepath;

		explicit Directory(utility::heap_string_utf8 && filepath)
			: filepath(std::move(filepath))
		{}
	};

	core::container::Collection
	<
		fd_t,
		utility::heap_storage_traits,
		utility::heap_storage<Directory>
	>
	directories;

	struct Alias
	{
		fd_t directory;

		int use_count;
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<Alias>
	>
	aliases;

	fd_t start_watch(fd_t notify_fd, const utility::heap_string_utf8 & filepath)
	{
		uint32_t mask = IN_CLOSE_WRITE | IN_CREATE | IN_DELETE | IN_ONLYDIR;
#if MODE_DEBUG
		mask |= IN_ATTRIB | IN_DELETE_SELF | IN_MODIFY | IN_MOVE_SELF | IN_MOVE;
#endif
		const fd_t fd = ::inotify_add_watch(notify_fd, filepath.data(), mask);
		debug_printline("starting watch ", fd, " of \"", filepath, "\"");
		if (fd == -1)
		{
			debug_verify(errno == ENOENT, "inotify_add_watch failed with errno ", errno);
		}
		return fd;
	}

	void stop_watch(fd_t notify_fd, fd_t watch_fd)
	{
		debug_printline("stopping watch ", watch_fd);
		debug_inform(::inotify_rm_watch(notify_fd, watch_fd) != -1, "failed with errno ", errno);
	}

	void clear_aliases()
	{
#if MODE_DEBUG
		for (const Alias & alias : aliases.get<Alias>())
		{
			debug_printline("removing alias prematurely \"", aliases.get_key(alias), "\" with ", alias.use_count, " users");
		}
#endif
		aliases.clear();
	}

	void clear_directories(fd_t notify_fd)
	{
		for (const Directory & directory : directories.get<Directory>())
		{
			debug_printline("stopping watch prematurely \"", directory.filepath, "\"");
			stop_watch(notify_fd, directories.get_key(directory));
		}
		directories.clear();
	}

	void clear_watches()
	{
#if MODE_DEBUG
		for (const ReadWatch & watch : watches.get<ReadWatch>())
		{
			debug_printline("removing read watch prematurely \"", watch.ptr->filepath, "\"");
		}
		for (const ScanWatch & watch : watches.get<ScanWatch>())
		{
			debug_printline("removing scan watch prematurely \"", watch.ptr->dirpath, "\"");
		}
		for (const ScanRecursiveWatch & watch : watches.get<ScanRecursiveWatch>())
		{
			debug_printline("removing scan watch prematurely \"", watch.ptr->dirpath, "\"");
		}
#endif
		watches.clear();
	}

	// todo remove
	void scan_directory_subdirs(utility::string_units_utf8 filepath, utility::heap_vector<utility::heap_string_utf8> & subdirs)
	{
		debug_verify(subdirs.try_emplace_back());

		utility::heap_string_utf8 pattern(filepath);
		for (ext::index index = 0; index < static_cast<ext::index>(subdirs.size()); index++)
		{
			if (!debug_verify(pattern.try_append(subdirs[index])))
				continue;

			DIR * const dir = ::opendir(pattern.data());
			if (debug_inform(dir != nullptr, "opendir(\"", pattern, "\") failed with errno ", errno))
			{
				debug_expression(errno = 0);

				while (struct dirent * const entry = ::readdir(dir))
				{
					// todo d_type not always supported, if so lstat is needed
					if (entry->d_type != DT_DIR)
						continue;

					if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
						continue;

					if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
						continue;

					if (debug_verify(subdirs.try_emplace_back()))
					{
						utility::heap_string_utf8 & subdir = ext::back(subdirs);
						if (!(debug_verify(subdir.try_append(subdirs[index])) &&
						      debug_verify(subdir.try_append(const_cast<const char *>(entry->d_name))) &&
						      debug_verify(subdir.try_push_back('/'))))
						{
							ext::pop_back(subdirs);
						}
					}
				}

				debug_assert(errno == 0, "readdir failed with errno ", errno);

				debug_verify(::closedir(dir) != -1, "failed with errno ", errno);
			}

			pattern.reduce(subdirs[index].size());
		}
	}

	Directory * get_directory(engine::Asset asset)
	{
		const auto alias_it = find(aliases, asset);
		if (alias_it == aliases.end())
			return nullptr;

		Alias * const alias = aliases.get<Alias>(alias_it);
		if (!debug_assert(alias))
			return nullptr;

		const auto directory_it = find(directories, alias->directory);
		if (!debug_assert(directory_it != directories.end()))
			return nullptr;

		Directory * const directory = directories.get<Directory>(directory_it);
		if (!debug_assert(directory))
			return nullptr;

		return directory;
	}

	Directory * get_or_create_directory(engine::Asset asset, fd_t notify_fd, utility::string_units_utf8 filepath)
	{
		const auto alias_it = find(aliases, asset);
		if (alias_it != aliases.end())
		{
			Alias * const alias = aliases.get<Alias>(alias_it);
			if (!debug_assert(alias))
				return nullptr;

			const auto directory_it = find(directories, alias->directory);
			if (!debug_assert(directory_it != directories.end()))
				return nullptr;

			Directory * const directory = directories.get<Directory>(directory_it);
			if (!debug_assert(directory))
				return nullptr;

			alias->use_count++;

			return directory;
		}
		else
		{
			utility::heap_string_utf8 filepath_null(filepath);
			const fd_t fd = start_watch(notify_fd, filepath_null);
			if (fd == -1)
				return nullptr;

			Directory * const directory = directories.emplace<Directory>(fd, std::move(filepath_null));
			if (!debug_verify(directory))
			{
				stop_watch(notify_fd, fd);
				return nullptr;
			}

			Alias * const alias = aliases.emplace<Alias>(asset, fd, 1);
			if (!debug_verify(alias))
			{
				directories.erase(find(directories, fd));
				stop_watch(notify_fd, fd);
				return nullptr;
			}

			return directory;
		}
	}

	void decrement_alias(fd_t notify_fd, engine::Asset asset)
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
			const fd_t fd = alias->directory;
			aliases.erase(alias_it);

			const auto directory_it = find(directories, fd);
			if (!debug_assert(directory_it != directories.end()))
				return;

#if MODE_DEBUG
			const Directory * const directory = directories.get<Directory>(directory_it);
			if (debug_assert(directory))
			{
				debug_inform(ext::empty(directory->reads));
				debug_inform(ext::empty(directory->missing_reads));
				debug_inform(ext::empty(directory->scans));
				debug_inform(ext::empty(directory->recursive_scans));
			}
#endif
			directories.erase(directory_it);

			stop_watch(notify_fd, fd);
		}
	}

	void add_recursive_scan(utility::heap_string_utf8 && filepath, fd_t notify_fd, const ext::heap_shared_ptr<engine::file::ScanData> & ptr)
	{
		utility::heap_string_utf8 pattern;
		if (!debug_verify(pattern.try_append(std::move(filepath))))
			return;

		utility::heap_vector<utility::heap_string_utf8> subdirs;
		if (!debug_verify(subdirs.try_emplace_back()))
			return;

		while (!ext::empty(subdirs))
		{
			utility::heap_string_utf8 subdir = ext::back(std::move(subdirs));
			ext::pop_back(subdirs);

			if (!debug_verify(pattern.try_append(subdir)))
				continue;

			debug_printline(pattern);
			const auto alias = engine::Asset(pattern);
			if (Directory * const directory = get_or_create_directory(alias, notify_fd, pattern))
			{
				bool using_directory = false;

				using_directory = debug_verify(directory->scans.try_emplace_back(ptr)) || using_directory;

				using_directory = debug_verify(directory->recursive_scans.try_emplace_back(ptr)) || using_directory;

				if (!using_directory)
				{
					decrement_alias(notify_fd, alias);
				}
			}

			DIR * const dir = ::opendir(pattern.data());
			if (debug_inform(dir != nullptr, "opendir(\"", pattern, "\") failed with errno ", errno))
			{
				debug_expression(errno = 0);

				while (struct dirent * const entry = ::readdir(dir))
				{
					// todo d_type not always supported, if so lstat is needed
					if (entry->d_type != DT_DIR)
						continue;

					if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
						continue;

					if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
						continue;

					if (debug_verify(subdirs.try_emplace_back()))
					{
						utility::heap_string_utf8 & newdir = ext::back(subdirs);
						if (!(debug_verify(newdir.try_append(subdir)) &&
						      debug_verify(newdir.try_append(const_cast<const char *>(entry->d_name))) &&
						      debug_verify(newdir.try_push_back('/'))))
						{
							ext::pop_back(subdirs);
						}
					}
				}

				debug_assert(errno == 0, "readdir failed with errno ", errno);

				debug_verify(::closedir(dir) != -1, "failed with errno ", errno);
			}

			pattern.reduce(subdir.size());
		}
	}

	void process_add_read(fd_t notify_fd, engine::Identity watch_id, ext::heap_shared_ptr<engine::file::ReadData> && ptr, bool report_missing)
	{
		if (!debug_verify(watches.emplace<ReadWatch>(watch_id, ptr)))
			return;

		const auto directory_split = utility::rfind(ptr->filepath, '/');
		if (!debug_assert(directory_split != ptr->filepath.end()))
			return;

		utility::string_units_utf8 filepath(ptr->filepath.begin(), directory_split + 1); // include '/'
		const auto alias = engine::Asset(filepath);
		if (Directory * const directory = get_or_create_directory(alias, notify_fd, filepath))
		{
			utility::string_units_utf8 filename(directory_split + 1, ptr->filepath.end());

			bool using_directory = false;

			using_directory = debug_verify(directory->reads.try_emplace_back(filename, ptr)) || using_directory;

			if (report_missing)
			{
				using_directory = debug_verify(directory->missing_reads.try_emplace_back(filename, std::move(ptr))) || using_directory;
			}

			if (!using_directory)
			{
				decrement_alias(notify_fd, alias);
			}
		}
	}

	void process_add_scan(fd_t notify_fd, engine::Identity watch_id, ext::heap_shared_ptr<engine::file::ScanData> && ptr, bool recurse_directories)
	{
		if (recurse_directories)
		{
			if (!debug_verify(watches.emplace<ScanRecursiveWatch>(watch_id, ptr)))
				return;

			add_recursive_scan(utility::heap_string_utf8(ptr->dirpath), notify_fd, ptr);
		}
		else
		{
			if (!debug_verify(watches.emplace<ScanWatch>(watch_id, ptr)))
				return;

			utility::string_units_utf8 filepath(ptr->dirpath);
			const auto alias = engine::Asset(filepath);
			if (Directory * const directory = get_or_create_directory(alias, notify_fd, filepath))
			{
				if (!debug_verify(directory->scans.try_emplace_back(std::move(ptr))))
				{
					decrement_alias(notify_fd, alias);
				}
			}
		}
	}

	void process_remove(fd_t notify_fd, engine::Identity watch_id)
	{
		const auto watch_it = find(watches, watch_id);
		if (!debug_verify(watch_it != watches.end()))
			return;

		watches.call(
			watch_it,
			[notify_fd](const ReadWatch & x)
			{
				const auto directory_split = utility::rfind(x.ptr->filepath, '/');
				if (!debug_assert(directory_split != x.ptr->filepath.end()))
					return;

				utility::string_units_utf8 filepath(x.ptr->filepath.begin(), directory_split + 1); // include '/'
				const auto alias = engine::Asset(filepath);
				if (Directory * const directory = get_directory(alias))
				{
					utility::string_units_utf8 filename(directory_split + 1, x.ptr->filepath.end());

					bool using_directory = false;

					const auto report_missing_it = ext::find_if(directory->missing_reads, fun::second == x.ptr);
					if (report_missing_it != directory->missing_reads.end())
					{
						using_directory = true;
						directory->missing_reads.erase(report_missing_it);
					}

					const auto read_it = ext::find_if(directory->reads, fun::second == x.ptr);
					if (debug_verify(read_it != directory->reads.end()))
					{
						using_directory = true;
						directory->reads.erase(read_it);
					}

					if (using_directory)
					{
						decrement_alias(notify_fd, alias);
					}
				}
			},
			[notify_fd](const ScanWatch & x)
			{
				utility::string_units_utf8 filepath(x.ptr->dirpath);
				const auto alias = engine::Asset(filepath);
				if (Directory * const directory = get_directory(alias))
				{
					const auto scan_it = ext::find(directory->scans, x.ptr);
					if (debug_verify(scan_it != directory->scans.end()))
					{
						directory->scans.erase(scan_it);
						decrement_alias(notify_fd, alias);
					}
				}
			},
			[notify_fd](const ScanRecursiveWatch & x)
			{
				utility::heap_vector<utility::heap_string_utf8> subdirs;
				scan_directory_subdirs(x.ptr->dirpath, subdirs);

				utility::heap_string_utf8 filepath = x.ptr->dirpath;
				for (const auto & subdir : subdirs)
				{
					if (!debug_verify(filepath.try_append(subdir)))
						continue;

					const auto alias = engine::Asset(filepath);
					if (Directory * const directory = get_directory(alias))
					{
						bool using_directory = false;

						const auto recursive_scan_it = ext::find(directory->recursive_scans, x.ptr);
						if (debug_inform(recursive_scan_it != directory->recursive_scans.end()))
						{
							using_directory = true;
							directory->recursive_scans.erase(recursive_scan_it);
						}

						const auto scan_it = ext::find(directory->scans, x.ptr);
						if (debug_inform(scan_it != directory->scans.end()))
						{
							using_directory = true;
							directory->scans.erase(scan_it);
						}

						if (using_directory)
						{
							decrement_alias(notify_fd, alias);
						}
					}

					filepath.reduce(subdir.size());
				}
			});

		watches.erase(watch_it);
	}

	core::async::thread_return thread_decl file_watch(core::async::thread_param /*arg*/)
	{
		const fd_t notify_fd = ::inotify_init1(IN_NONBLOCK);
		if (!debug_verify(notify_fd >= 0, "inotify failed with ", errno))
			return core::async::thread_return{};

		auto panic =
			[notify_fd]()
			{
				// todo fix leak of unread messages
				clear_aliases();
				clear_directories(notify_fd);
				clear_watches();
				::close(notify_fd);
				return core::async::thread_return{};
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

			if (!debug_verify((fds[0].revents & ~(POLLIN | POLLHUP)) == 0))
				return panic();

			bool terminate = false;

			if (fds[0].revents & (POLLIN | POLLHUP))
			{
				Message message;
				while (true)
				{
					const auto message_result = ::read(message_pipe[0], &message, sizeof message);
					if (message_result <= 0)
					{
						if (message_result == 0) // pipe closed
						{
							terminate = true;
							break;
						}

						if (!debug_verify(errno == EAGAIN))
							return panic();

						break;
					}

					switch (message.type)
					{
					case Message::ADD_READ:
						process_add_read(notify_fd, message.id, ext::heap_shared_ptr<engine::file::ReadData>(*message.read_ptr), message.flags == Message::REPORT_MISSING);
						break;
					case Message::ADD_SCAN:
						process_add_scan(notify_fd, message.id, ext::heap_shared_ptr<engine::file::ScanData>(*message.scan_ptr), message.flags == Message::RECURSE_DIRECTORIES);
						break;
					case Message::REMOVE:
						process_remove(notify_fd, message.id);
						break;
					}
				}
			}

			if (fds[1].revents & POLLIN)
			{
				std::aligned_storage_t<4096, alignof(struct inotify_event)> buffer; // arbitrary

				while (true)
				{
					const ext::ssize n = ::read(fds[1].fd, &buffer, sizeof buffer);
					if (!debug_assert(n != 0, "unexpected eof"))
						return panic();

					if (n < 0)
					{
						if (debug_verify(errno == EAGAIN))
							break;

						return panic();
					}

					utility::heap_vector<const Directory *, utility::heap_vector<utility::heap_string_utf8>> changed_directories;
					auto get_or_create_change = [&](const Directory * directory)
					{
						const auto it = ext::find_if(changed_directories, fun::get<0> == directory);
						if (it != changed_directories.end())
							return it;

						if (!debug_verify(changed_directories.try_emplace_back(directory, utility::heap_vector<utility::heap_string_utf8>())))
							return it;

						return changed_directories.end() - 1;
					};

					const char * const begin = reinterpret_cast<const char *>(&buffer);
					const char * const end = begin + n;
					for (const char * ptr = begin; ptr != end;)
					{
						const struct inotify_event * const event = reinterpret_cast<const struct inotify_event *>(ptr);
						if (!debug_assert((sizeof(struct inotify_event) + event->len) % alignof(struct inotify_event) == 0))
							return panic();

						ptr += sizeof(struct inotify_event) + event->len;

#if MODE_DEBUG
						debug_printline("event ", event->wd, " ", event->name);
						if (event->mask & IN_ACCESS) { debug_printline("event IN_ACCESS"); }
						if (event->mask & IN_ATTRIB) { debug_printline("event IN_ATTRIB"); }
						if (event->mask & IN_CLOSE_WRITE) { debug_printline("event IN_CLOSE_WRITE"); }
						if (event->mask & IN_CLOSE_NOWRITE) { debug_printline("event IN_CLOSE_NOWRITE"); }
						if (event->mask & IN_CREATE) { debug_printline("event IN_CREATE"); }
						if (event->mask & IN_DELETE) { debug_printline("event IN_DELETE"); }
						if (event->mask & IN_DELETE_SELF) { debug_printline("event IN_DELETE_SELF"); }
						if (event->mask & IN_MODIFY) { debug_printline("event IN_MODIFY"); }
						if (event->mask & IN_MOVE_SELF) { debug_printline("event IN_MOVE_SELF"); }
						if (event->mask & IN_MOVED_FROM) { debug_printline("event IN_MOVED_FROM"); }
						if (event->mask & IN_MOVED_TO) { debug_printline("event IN_MOVED_TO"); }
						if (event->mask & IN_OPEN) { debug_printline("event IN_OPEN"); }
						if (event->mask & IN_IGNORED) { debug_printline("event IN_IGNORED"); }
						if (event->mask & IN_ISDIR) { debug_printline("event IN_ISDIR"); }
						if (event->mask & IN_Q_OVERFLOW) { debug_printline("event IN_Q_OVERFLOW"); }
						if (event->mask & IN_UNMOUNT) { debug_printline("event IN_UNMOUNT"); }
#endif

						const auto directory_it = find(directories, event->wd);
						if (!debug_inform(directory_it != directories.end(), "cannot find directory ", event->wd))
							continue; // must have been removed :shrug:

						Directory * const directory = directories.get<Directory>(directory_it);
						if (!debug_assert(directory))
							continue;

						if (event->mask & IN_CREATE)
						{
							if (event->mask & IN_ISDIR)
							{
								if (!ext::empty(directory->recursive_scans))
								{
									utility::heap_string_utf8 filepath;
									if (debug_verify(filepath.try_append(directory->filepath)) &&
									    debug_verify(filepath.try_append(event->name)) &&
									    debug_verify(filepath.try_push_back('/')))
									{
										for (auto && scan : directory->recursive_scans)
										{
											add_recursive_scan(utility::heap_string_utf8(filepath), notify_fd, scan);

											engine::file::post_work(engine::file::ScanRecursiveWork{scan});
										}
									}
								}
							}
							else
							{
								const auto change_it = get_or_create_change(directory);
								if (change_it != changed_directories.end())
								{
									if (debug_verify(std::get<1>(*change_it).try_emplace_back()))
									{
										auto & file = ext::back(std::get<1>(*change_it));
										if (!(debug_verify(file.try_push_back('+')) &&
										      debug_verify(file.try_append(event->name))))
										{
											ext::pop_back(std::get<1>(*change_it));
										}
									}
								}
							}
						}

						if (event->mask & IN_DELETE)
						{
							const auto change_it = get_or_create_change(directory);
							if (change_it != changed_directories.end())
							{
								if (debug_verify(std::get<1>(*change_it).try_emplace_back()))
								{
									auto & file = ext::back(std::get<1>(*change_it));
									if (!(debug_verify(file.try_push_back('-')) &&
									      debug_verify(file.try_append(event->name))))
									{
										ext::pop_back(std::get<1>(*change_it));
									}
								}
							}
							for (auto && read : directory->missing_reads)
							{
								if (read.first == event->name)
								{
									engine::file::post_work(engine::file::FileMissingWork{read.second});
								}
							}
						}

						if (event->mask & IN_CLOSE_WRITE)
						{
							for (auto && read : directory->reads)
							{
								if (read.first == event->name)
								{
									engine::file::post_work(engine::file::FileReadWork{read.second});
								}
							}
						}

						if (event->mask & IN_DELETE_SELF)
						{
							const auto changed_directory_it = ext::find_if(changed_directories, fun::first == directory);
							if (changed_directory_it != changed_directories.end())
							{
								auto && changed = *changed_directory_it;
								for (auto && scan : changed.first->scans)
								{
									engine::file::post_work(engine::file::ScanChangeWork{scan, changed.first->filepath, changed.second});
								}
								changed_directories.erase(changed_directory_it);
							}
							const auto alias = engine::Asset(directory->filepath);
							decrement_alias(notify_fd, alias);
						}
					}

					for (auto && changed : changed_directories)
					{
						for (auto && scan : changed.first->scans)
						{
							engine::file::post_work(engine::file::ScanChangeWork{scan, changed.first->filepath, changed.second});
						}
					}
				}
			}

			if (terminate)
				return panic();
		}
	}
}

namespace engine
{
	namespace file
	{
		void initialize_watch()
		{
			if (!debug_verify(::pipe2(message_pipe, O_NONBLOCK) != -1, "failed with errno ", errno))
				return;

			watch_thread = core::async::Thread(file_watch, nullptr);
		}

		void deinitialize_watch()
		{
			if (!debug_verify(watch_thread.valid()))
				return;

			::close(message_pipe[1]);

			watch_thread.join();

			::close(message_pipe[0]);
		}

		void add_file_watch(engine::Identity id, ext::heap_shared_ptr<ReadData> ptr, bool report_missing)
		{
			if (!debug_assert(watch_thread.valid()))
				return;

			const Message message(Message::ADD_READ, report_missing ? Message::REPORT_MISSING : Message::NONE, id, ptr.detach());
			static_assert(sizeof message <= PIPE_BUF, "writes up to PIPE_BUF are atomic (see pipe(7))");
			if (!debug_verify(ext::write_some_nonzero(message_pipe[1], &message, sizeof message) == sizeof message))
			{
				static_cast<void>(ext::heap_shared_ptr<ReadData>(*message.read_ptr));
			}
		}

		void add_scan_watch(engine::Identity id, ext::heap_shared_ptr<ScanData> ptr, bool recurse_directories)
		{
			if (!debug_assert(watch_thread.valid()))
				return;

			const Message message(Message::ADD_SCAN, recurse_directories ? Message::RECURSE_DIRECTORIES : Message::NONE, id, ptr.detach());
			static_assert(sizeof message <= PIPE_BUF, "writes up to PIPE_BUF are atomic (see pipe(7))");
			if (!debug_verify(ext::write_some_nonzero(message_pipe[1], &message, sizeof message) == sizeof message))
			{
				static_cast<void>(ext::heap_shared_ptr<ScanData>(*message.scan_ptr));
			}
		}

		void remove_watch(engine::Identity id)
		{
			if (!debug_assert(watch_thread.valid()))
				return;

			const Message message(Message::REMOVE, Message::NONE, id);
			static_assert(sizeof message <= PIPE_BUF, "writes up to PIPE_BUF are atomic (see pipe(7))");
			debug_verify(ext::write_some_nonzero(message_pipe[1], &message, sizeof message) == sizeof message);
		}
	}
}

#endif
