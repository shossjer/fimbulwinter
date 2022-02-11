#include "config.h"

#if FILE_SYSTEM_USE_POSIX

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"
#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/Asset.hpp"
#include "engine/file/system.hpp"
#include "engine/file/watch/watch.hpp"
#include "engine/HashTable.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"
#include "utility/ext/unistd.hpp"
#include "utility/optional.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/spinlock.hpp"

#include "ful/string_modify.hpp"
#include "ful/string_search.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mutex>

static_hashes("_working directory_");

namespace
{
	struct Directory
	{
		ful::heap_string_utf8 dirpath;

		ext::ssize share_count;

		explicit Directory(ful::heap_string_utf8 && dirpath)
			: dirpath(std::move(dirpath))
			, share_count(0)
		{}
	};

	struct TemporaryDirectory
	{
		ful::heap_string_utf8 dirpath;

		explicit TemporaryDirectory(ful::heap_string_utf8 && dirpath)
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

			int pipe[2];
			core::async::Thread thread;

			const ful::heap_string_utf8 & get_dirpath(decltype(directories)::const_iterator it)
			{
				return directories.call(it, [](const auto & x) -> const ful::heap_string_utf8 & { return x.dirpath; });
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
	void scan_directory(const ful::heap_string_utf8 & dirpath, bool recurse, ful::heap_string_utf8 & files)
	{
		utility::heap_vector<ful::heap_string_utf8> subdirs;
		if (!debug_verify(subdirs.try_emplace_back()))
			return;

		ful::heap_string_utf8 pattern;
		if (!debug_verify(ful::append(pattern, dirpath)))
			return; // error

		while (!ext::empty(subdirs))
		{
			auto subdir = ext::back(std::move(subdirs));
			ext::pop_back(subdirs);

			if (!debug_verify(ful::append(pattern, subdir)))
				return; // error

			DIR * const dir = ::opendir(pattern.data());
			if (!debug_verify(dir != nullptr, "opendir failed with errno ", errno))
				return;

			errno = 0;

			while (struct dirent * const entry = ::readdir(dir))
			{
				// todo d_type not always supported, if so lstat is needed
				if (entry->d_type == DT_DIR)
				{
					if (recurse)
					{
						if (entry->d_name[0] == '.' && entry->d_name[1] == '\0')
							continue;

						if (entry->d_name[0] == '.' && entry->d_name[1] == '.' && entry->d_name[2] == '\0')
							continue;

						if (!debug_verify(subdirs.try_emplace_back()))
							return; // error

						auto & dir = ext::back(subdirs);

						if (!debug_verify(ful::append(dir, subdir)))
							return; // error

						if (!debug_verify(ful::append(dir, entry->d_name + 0, ful::strend(entry->d_name))))
							return; // error

						if (!debug_verify(ful::push_back(dir, ful::char8{'/'})))
							return; // error
					}
				}
				else
				{
					if (!debug_verify(ful::append(files, subdir)))
						return; // error

					if (!debug_verify(ful::append(files, entry->d_name + 0, ful::strend(entry->d_name))))
						return; // error

					if (!debug_verify(ful::push_back(files, ful::char8{';'})))
						return; // error
				}
			}

			debug_verify(errno == 0, "readdir failed with errno ", errno);

			debug_verify(::closedir(dir) != -1, "failed with errno ", errno);

			ful::reduce(pattern, pattern.begin() + dirpath.size());
		}

		if (!empty(files))
		{
			files.reduce(files.end() - 1); // trailing ;
		}
	}

	bool read_file(engine::file::system_impl & impl, ful::heap_string_utf8 & filepath, std::uint32_t root, engine::file::read_callback * callback, utility::any & data)
	{
		// note updating access time takes time, so let's not (O_NOATIME)
		const int fd = ::open(filepath.data(), O_RDONLY | O_NOATIME);
		if (fd == -1)
		{
			debug_verify(errno == ENOENT, "open(\"", filepath, "\", O_RDONLY) failed with errno ", errno);
			return false;
		}

		// todo can this lock be acquired on open?
		while (::flock(fd, LOCK_SH) != 0)
		{
			if (!debug_verify(errno == EINTR))
			{
				break;
			}
		}

		ful::cstr_utf8 relpath_(filepath.data() + root, filepath.data() + filepath.size());
		core::ReadStream stream(
			[](void * dest, ext::usize n, void * data)
			{
				const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

				return ext::read_some_nonzero(fd, dest, n);
			},
			reinterpret_cast<void *>(fd),
			relpath_);

		engine::file::system filesystem(impl);
		callback(filesystem, std::move(stream), data);
		filesystem.detach();

		debug_verify(::close(fd) != -1, "failed with errno ", errno);
		return true;
	}

	bool write_file(engine::file::system_impl & impl, ful::heap_string_utf8 & filepath, std::uint32_t root, engine::file::write_callback * callback, utility::any & data, bool append, bool overwrite)
	{
		// todo define _FILE_OFFSET_BITS 64 (see open(2))

		int flags = O_WRONLY | O_CREAT | O_EXCL;
		int mode = 0664;
		if (append)
		{
			flags = O_WRONLY | O_APPEND;
		}
		else if (overwrite)
		{
			flags = O_WRONLY | O_CREAT | O_TRUNC;
		}

		const int fd = ::open(filepath.data(), flags, mode);
		if (fd == -1)
		{
			debug_verify(errno == EEXIST, "open \"", filepath, "\" failed with errno ", errno);
			return false;
		}

		// todo can this lock be acquired on open?
		while (::flock(fd, LOCK_EX) != 0)
		{
			if (!debug_verify(errno == EINTR))
			{
				break;
			}
		}

		ful::cstr_utf8 relpath_(filepath.data() + root, filepath.data() + filepath.size());
		core::WriteStream stream(
			[](const void * src, ext::usize n, void * data)
			{
				const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

				return ext::write_some_nonzero(fd, src, n);
			},
			reinterpret_cast<void *>(fd),
			relpath_);

		engine::file::system filesystem(impl);
		callback(filesystem, std::move(stream), std::move(data));
		filesystem.detach();

		debug_verify(::close(fd) != -1, "failed with errno ", errno);
		return true;
	}

	void purge_temporary_directory(const ful::heap_string_utf8 & filepath)
	{
		debug_printline("removing temporary directory \"", filepath, "\"");
		// todo replace nftw
		debug_verify(
			::nftw(
				filepath.data(),
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
				FTW_DEPTH | FTW_MOUNT | FTW_PHYS)
			!= -1, "failed to remove temporary directory \"", filepath, "\"");
	}

	const ful::unit_utf8 * check_filepath(const ful::unit_utf8 * begin, const ful::unit_utf8 * end)
	{
		// todo disallow windows absolute paths

		// todo report \\ as error

		while (true)
		{
			const auto slash = ful::find(begin, end, ful::char8{'/'});
			if (slash == begin)
				return slash; // note this disallows linux absolute paths

			if (*begin == '.')
			{
				auto next = begin + 1;
				if (next == slash)
					return begin; // disallow .

				if (*next == '.')
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

	bool validate_filepath(ful::view_utf8 str)
	{
		return check_filepath(str.begin(), str.end()) == str.end();
	}

	bool add_file(ful::heap_string_utf8 & files, ful::view_utf8 subdir, ful::view_utf8 filename)
	{
		auto begin = files.begin();
		const auto end = files.end();
		if (begin != end)
		{
			while (true)
			{
				const auto split = ful::find(begin, end, ful::char8{';'});
				if (ful::starts_with(begin, end, subdir))
				{
					const auto dir_skip = begin + subdir.size();
					if (ful::view_utf8(dir_skip, split) == filename)
						return false; // already added
				}

				if (split == end)
					break;

				begin = split + 1; // skip ;
			}

			if (!debug_verify(ful::push_back(files, ful::char8{';'})))
				return false;
		}

		if (!(debug_verify(ful::append(files, subdir)) &&
		      debug_verify(ful::append(files, filename))))
		{
			files.erase(begin == end ? files.begin() : ful::rfind(files, ful::char8{';'}), files.end());
			return false;
		}

		return true;
	}

	bool remove_file(ful::heap_string_utf8 & files, ful::view_utf8 subdir, ful::view_utf8 filename)
	{
		auto begin = files.begin();
		const auto end = files.end();

		auto prev_split = begin;
		while (true)
		{
			const auto split = ful::find(begin, end, ful::char8{';'});
			if (split == end)
				break;

			const auto next_begin = split + 1; // skip ;

			if (ful::starts_with(begin, split, subdir) && ful::view_utf8(begin + subdir.size(), split) == filename)
			{
				files.erase(begin, next_begin);

				return true;
			}

			prev_split = split;
			begin = next_begin;
		}

		if (ful::starts_with(begin, end, subdir) && ful::view_utf8(begin + subdir.size(), end) == filename)
		{
			files.erase(prev_split, end);

			return true;
		}

		return false; // already removed
	}

	void remove_duplicates(ful::heap_string_utf8 & files, ful::view_utf8 duplicates)
	{
		auto duplicates_begin = duplicates.begin();
		const auto duplicates_end = duplicates.end();

		if (duplicates_begin != duplicates_end)
		{
			while (true)
			{
				const auto duplicates_split = ful::find(duplicates_begin, duplicates_end, ful::char8{';'});
				if (!debug_assert(duplicates_split != duplicates_begin, "unexpected file without name"))
					return; // error

				remove_file(files, ful::view_utf8{}, ful::view_utf8(duplicates_begin, duplicates_split));

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

						ful::cstr_utf8 relpath_(read_data.filepath.data() + read_data.root, read_data.filepath.data() + read_data.filepath.size());
						core::ReadStream stream(relpath_);

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

						if (read_file(read_data.impl, read_data.filepath, read_data.root, read_data.callback, read_data.data))
						{
						}
						else
						{
							ful::cstr_utf8 relpath_(read_data.filepath.data() + read_data.root, read_data.filepath.data() + read_data.filepath.size());
							core::ReadStream stream(relpath_);

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

						// todo garbage callback if copy fails
						ful::heap_string_utf8 old_files;
						if (!debug_verify(ful::copy(scan_data.files, old_files)))
							return; // error

						ful::view_utf8 subdir(work.filepath.begin() + scan_data.dirpath.size(), work.filepath.end());

						for (const auto & file : work.files)
						{
							switch (file.data()[0])
							{
							case '+':
								add_file(scan_data.files, subdir, ful::view_utf8(file.begin() + 1, file.end()));
								break;
							case '-':
								remove_file(scan_data.files, subdir, ful::view_utf8(file.begin() + 1, file.end()));
								break;
							default:
								debug_unreachable("unknown file change");
							}
						}

						remove_duplicates(old_files, ful::view_utf8(scan_data.files));

						ful::heap_string_utf8 existing_files;
						if (!debug_verify(ful::copy(scan_data.files, existing_files)))
							return; // error

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, std::move(existing_files), std::move(old_files), scan_data.data);
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

						ful::heap_string_utf8 removed_files = std::move(scan_data.files);
						scan_directory(scan_data.dirpath, false, scan_data.files);

						remove_duplicates(removed_files, ful::view_utf8(scan_data.files));

						ful::heap_string_utf8 existing_files;
						if (!debug_verify(ful::copy(scan_data.files, existing_files)))
							return; // error

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, std::move(existing_files), std::move(removed_files), scan_data.data);
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

						ful::heap_string_utf8 removed_files = std::move(scan_data.files);
						scan_directory(scan_data.dirpath, true, scan_data.files);

						remove_duplicates(removed_files, ful::view_utf8(scan_data.files));

						ful::heap_string_utf8 existing_files;
						if (!debug_verify(ful::copy(scan_data.files, existing_files)))
							return; // error

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, std::move(existing_files), std::move(removed_files), scan_data.data);
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
							ful::cstr_utf8 relpath_(write_data.filepath.data() + write_data.root, write_data.filepath.data() + write_data.filepath.size());
							core::WriteStream stream(relpath_);

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
	struct RegisterDirectory
	{
		engine::Hash alias;
		ful::heap_string_utf8 filepath;
		engine::Hash parent;
	};

	struct RegisterTemporaryDirectory
	{
		engine::Hash alias;
	};

	struct UnregisterDirectory
	{
		engine::Hash alias;
	};

	struct Read
	{
		engine::Token id;
		engine::Hash directory;
		ful::heap_string_utf8 filepath;
		engine::Hash strand;
		engine::file::read_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct RemoveWatch
	{
		engine::Token id;
	};

	struct Scan
	{
		engine::Token id;
		engine::Hash directory;
		engine::Hash strand;
		engine::file::scan_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct Write
	{
		engine::Hash directory;
		ful::heap_string_utf8 filepath;
		engine::Hash strand;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	void process_register_directory(engine::file::system_impl & system_impl, engine::file::watch_impl & /*watch_impl*/, void * data)
	{
		auto & x = *static_cast<RegisterDirectory *>(data);

		if (!debug_verify(find(system_impl.aliases, engine::Token(x.alias)) == system_impl.aliases.end()))
			return; // error

		const auto parent_alias_it = find(system_impl.aliases, engine::Token(x.parent));
		if (!debug_verify(parent_alias_it != system_impl.aliases.end()))
			return; // error

		if (!debug_verify(validate_filepath(ful::view_utf8(x.filepath))))
			return; // error

		const auto parent_directory_it = find(system_impl.directories, system_impl.aliases.get<Alias>(parent_alias_it)->directory);
		if (!debug_assert(parent_directory_it != system_impl.directories.end()))
			return;

		const auto & dirpath = system_impl.get_dirpath(parent_directory_it);

		ful::heap_string_utf8 filepath;
		if (!debug_verify(ful::append(filepath, dirpath)))
			return; // error

		if (!debug_verify(ful::append(filepath, x.filepath)))
			return; // error

		if (filepath.end()[-1] != '/')
		{
			if (!debug_verify(ful::push_back(filepath, ful::char8{'/'})))
				return; // error
		}

		const auto directory_asset = engine::Asset(filepath);
		const auto directory_it = find(system_impl.directories, engine::Token(directory_asset));
		if (directory_it != system_impl.directories.end())
		{
			const auto directory_ptr = system_impl.directories.get<Directory>(directory_it);
			if (!debug_verify(directory_ptr))
				return;

			directory_ptr->share_count++;
		}
		else
		{
			if (!debug_verify(system_impl.directories.emplace<Directory>(engine::Token(directory_asset), std::move(filepath))))
				return; // error
		}

		if (!debug_verify(system_impl.aliases.emplace<Alias>(engine::Token(x.alias), engine::Token(directory_asset))))
			return; // error
	}

	void process_register_temporary_directory(engine::file::system_impl & system_impl, engine::file::watch_impl & /*watch_impl*/, void * data)
	{
		auto & x = *static_cast<RegisterTemporaryDirectory *>(data);

		if (!debug_verify(find(system_impl.aliases, engine::Token(x.alias)) == system_impl.aliases.end()))
			return; // error

		constexpr ful::cstr_utf8 filepath_template = ful::cstr_utf8(P_tmpdir "/unnamed-XXXXXX"); // todo project name
		ful::heap_string_utf8 filepath;
		if (!debug_verify(ful::reserve(filepath, filepath_template.size() + 1)))
			return; // error

		if (!debug_verify(ful::assign(filepath, filepath_template)))
			return; // error impossible?

		if (!debug_verify(::mkdtemp(filepath.c_str()) != nullptr, "failed with errno ", errno, " : ", filepath))
			return; // error

		debug_printline("created temporary directory \"", filepath, "\"");

		if (!debug_verify(ful::push_back(filepath, ful::char8{'/'})))
		{
			purge_temporary_directory(filepath);
			return; // error
		}

		const auto directory_asset = engine::Asset(filepath);
		if (!debug_verify(system_impl.directories.emplace<TemporaryDirectory>(engine::Token(directory_asset), std::move(filepath))))
		{
			purge_temporary_directory(filepath);
			return; // error
		}

		if (!debug_verify(system_impl.aliases.emplace<Alias>(engine::Token(x.alias), engine::Token(directory_asset))))
			return; // error
	}

	void process_unregister_directory(engine::file::system_impl & system_impl, engine::file::watch_impl & /*watch_impl*/, void * data)
	{
		auto & x = *static_cast<UnregisterDirectory *>(data);

		const auto alias_it = find(system_impl.aliases, engine::Token(x.alias));
		if (!debug_verify(alias_it != system_impl.aliases.end(), "alias does not exist"))
			return; // error

		const auto alias_ptr = system_impl.aliases.get<Alias>(alias_it);
		if (!debug_assert(alias_ptr))
			return;

		const auto directory_it = find(system_impl.directories, alias_ptr->directory);
		if (!debug_assert(directory_it != system_impl.directories.end()))
			return;

		struct
		{
			bool operator () (Directory & x) { x.share_count--; return x.share_count < 0; }
			bool operator () (TemporaryDirectory & x)
			{
				purge_temporary_directory(x.dirpath);
				return true;
			}
		} detach_alias;

		const auto directory_is_unused = system_impl.directories.call(directory_it, detach_alias);
		if (directory_is_unused)
		{
			system_impl.directories.erase(directory_it);
		}

		system_impl.aliases.erase(alias_it);
	}

	void process_read(engine::file::system_impl & system_impl, engine::file::watch_impl & watch_impl, void * data)
	{
		auto & x = *static_cast<Read *>(data);

		const auto alias_it = find(system_impl.aliases, engine::Token(x.directory));
		if (!debug_verify(alias_it != system_impl.aliases.end()))
			return; // error

		const auto alias_ptr = system_impl.aliases.get<Alias>(alias_it);
		if (!debug_assert(alias_ptr))
			return;

		const auto directory_it = find(system_impl.directories, alias_ptr->directory);
		if (!debug_assert(directory_it != system_impl.directories.end()))
			return;

		const auto & dirpath = system_impl.get_dirpath(directory_it);

		ful::heap_string_utf8 filepath;
		if (!debug_verify(ful::append(filepath, dirpath)))
			return; // error

		if (!debug_verify(ful::append(filepath, x.filepath)))
			return; // error

		ext::heap_shared_ptr<engine::file::ReadData> data_ptr(utility::in_place, system_impl, std::move(filepath), static_cast<std::uint32_t>(dirpath.size()), x.strand, x.callback, std::move(x.data));
		if (!debug_verify(data_ptr))
			return; // error

		if (x.mode & engine::file::flags::ADD_WATCH)
		{
			engine::file::add_file_watch(watch_impl, x.id, data_ptr, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING));
		}

		engine::file::post_work(engine::file::FileReadWork{std::move(data_ptr)});
	}

	void process_remove_watch(engine::file::system_impl & /*system_impl*/, engine::file::watch_impl & watch_impl, void * data)
	{
		auto & x = *static_cast<RemoveWatch *>(data);

		engine::file::remove_watch(watch_impl, x.id);
	}

	void process_scan(engine::file::system_impl & system_impl, engine::file::watch_impl & watch_impl, void * data)
	{
		auto & x = *static_cast<Scan *>(data);

		const auto alias_it = find(system_impl.aliases, engine::Token(x.directory));
		if (!debug_verify(alias_it != system_impl.aliases.end()))
			return; // error

		const auto alias_ptr = system_impl.aliases.get<Alias>(alias_it);
		if (!debug_assert(alias_ptr))
			return;

		const auto directory_it = find(system_impl.directories, alias_ptr->directory);
		if (!debug_assert(directory_it != system_impl.directories.end()))
			return;

		ful::heap_string_utf8 dirpath;
		if (!debug_verify(ful::copy(system_impl.get_dirpath(directory_it), dirpath)))
			return; // error

		ext::heap_shared_ptr<engine::file::ScanData> call_ptr(utility::in_place, system_impl, std::move(dirpath), x.directory, x.strand, x.callback, std::move(x.data), ful::heap_string_utf8());
		if (!debug_verify(call_ptr))
			return; // error

		if (x.mode & engine::file::flags::ADD_WATCH)
		{
			engine::file::add_scan_watch(watch_impl, x.id, call_ptr, static_cast<bool>(x.mode & engine::file::flags::RECURSE_DIRECTORIES));
		}

		if (x.mode & engine::file::flags::RECURSE_DIRECTORIES)
		{
			engine::file::post_work(engine::file::ScanRecursiveWork{std::move(call_ptr)});
		}
		else
		{
			engine::file::post_work(engine::file::ScanOnceWork{std::move(call_ptr)});
		}
	}

	void process_write(engine::file::system_impl & system_impl, engine::file::watch_impl & /*watch_impl*/, void * data)
	{
		auto & x = *static_cast<Write *>(data);

		const auto alias_it = find(system_impl.aliases, engine::Token(x.directory));
		if (!debug_verify(alias_it != system_impl.aliases.end()))
			return; // error

		const auto alias_ptr = system_impl.aliases.get<Alias>(alias_it);
		if (!debug_assert(alias_ptr))
			return;

		const auto directory_it = find(system_impl.directories, alias_ptr->directory);
		if (!debug_assert(directory_it != system_impl.directories.end()))
			return;

		const auto & dirpath = system_impl.get_dirpath(directory_it);

		ful::heap_string_utf8 filepath;
		if (!debug_verify(ful::append(filepath, dirpath)))
			return; // error

		if (!debug_verify(ful::append(filepath, x.filepath)))
			return; // error

		ext::heap_shared_ptr<engine::file::WriteData> ptr(utility::in_place, system_impl, std::move(filepath), static_cast<std::uint32_t>(dirpath.size()), x.strand, x.callback, std::move(x.data), static_cast<bool>(x.mode & engine::file::flags::APPEND_EXISTING), static_cast<bool>(x.mode & engine::file::flags::OVERWRITE_EXISTING));
		if (!debug_verify(ptr))
			return; // error

		if (x.mode & engine::file::flags::CREATE_DIRECTORIES)
		{
			auto begin = ptr->filepath.begin() + dirpath.size();
			const auto end = ptr->filepath.end();

			while (true)
			{
				const auto slash = ful::find(begin, end, ful::char8{'/'});
				if (slash == end)
					break;

				*slash = ful::char8{};

				if (::mkdir(ptr->filepath.data(), 0775) != 0)
				{
					if (!debug_verify(errno == EEXIST))
						return; // error

					struct stat buf;
					if (!debug_verify(stat(ptr->filepath.data(), &buf) == 0))
						return; // error

					if (!debug_verify(S_ISDIR(buf.st_mode)))
						return; // error
				}

				*slash = ful::char8{'/'};

				begin = slash + 1;
			}
		}

		engine::file::post_work(engine::file::FileWriteWork{std::move(ptr)});
	}

	struct Message
	{
		void (* callback)(engine::file::system_impl & system_impl, engine::file::watch_impl & watch_impl, void * ptr);
		void * ptr; // todo solve without raw pointers
	};
	static_assert(sizeof(Message) <= PIPE_BUF, "writes up to PIPE_BUF are atomic (see pipe(7))");

	core::async::thread_return thread_decl file_watch(core::async::thread_param arg)
	{
		engine::file::system_impl & impl = *static_cast<engine::file::system_impl *>(arg);

		engine::file::watch_impl watch_impl;

		struct pollfd fds[2] = {
			{impl.pipe[0], POLLIN, 0},
			{watch_impl.fd, POLLIN, 0},
		};

		while (true)
		{
			const int n = ::poll(fds, sizeof fds / sizeof fds[0], -1);
			if (n < 0)
			{
				if (debug_verify(errno == EINTR))
					continue;

				return core::async::thread_return{};
			}

			debug_assert(0 < n, "unexpected timeout");

			if (!debug_verify((fds[0].revents & ~(POLLIN | POLLHUP)) == 0))
				return core::async::thread_return{};

			bool terminate = false;

			if (fds[0].revents & (POLLIN | POLLHUP))
			{
				Message message;
				while (true)
				{
					const auto message_result = ::read(impl.pipe[0], &message, sizeof message);
					if (message_result <= 0)
					{
						if (message_result == 0) // pipe closed
						{
							terminate = true;
							break;
						}

						if (!debug_verify(errno == EAGAIN))
							return core::async::thread_return{};

						break;
					}

					message.callback(impl, watch_impl, message.ptr);
				}
			}

			if (fds[1].revents & POLLIN)
			{
				engine::file::process_watch(watch_impl);
			}

			if (terminate)
				return core::async::thread_return{};
		}
	}
}

namespace engine
{
	namespace file
	{
		directory directory::working_directory()
		{
			directory dir;

			char * malloc_str = ::get_current_dir_name();
			if (!debug_verify(malloc_str, "get_current_dir_name failed with errno ", errno))
				return dir;

			const auto len = ful::strlen(malloc_str) + 1; // one extra for trailing /
			if (debug_verify(ful::assign(dir.filepath_, malloc_str, malloc_str + len)))
			{
				dir.filepath_.data()[len - 1] = '/';
			}

			::free(malloc_str);

			return dir;
		}

		void system::destruct(system_impl & impl)
		{
			if (!debug_verify(impl.thread.valid()))
				return;

			::close(impl.pipe[1]);

			impl.thread.join();

			::close(impl.pipe[0]);

			destroy_impl(impl);
		}

		system_impl * system::construct(engine::task::scheduler & taskscheduler, directory && root)
		{
			system_impl * const impl = create_impl();
			if (debug_verify(impl))
			{
				impl->taskscheduler = &taskscheduler;

				const auto root_asset = engine::Asset(root.filepath_);

				if (!debug_verify(impl->directories.emplace<Directory>(engine::Token(root_asset), std::move(root.filepath_))))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				impl->strand = root_asset; // the strand ought to be something unique, this is probably good enough

				if (!debug_verify(impl->aliases.emplace<Alias>(engine::Token(engine::file::working_directory), engine::Token(root_asset))))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				if (!debug_verify(::pipe2(impl->pipe, O_NONBLOCK) != -1, "failed with errno ", errno))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				impl->thread = core::async::Thread(file_watch, impl);
				if (!impl->thread.valid())
				{
					::close(impl->pipe[0]);
					::close(impl->pipe[1]);

					destroy_impl(*impl);
					return nullptr;
				}
			}
			return impl;
		}

		void register_directory(system & system, engine::Hash name, ful::heap_string_utf8 && filepath, engine::Hash parent)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new RegisterDirectory{name, std::move(filepath), parent}; // todo

			const Message message{process_register_directory, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}

		void register_temporary_directory(system & system, engine::Hash name)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new RegisterTemporaryDirectory{name}; // todo

			const Message message{process_register_temporary_directory, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}

		void unregister_directory(system & system, engine::Hash name)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new UnregisterDirectory{name}; // todo

			const Message message{process_unregister_directory, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}

		void read(
			system & system,
			engine::Token id,
			engine::Hash directory,
			ful::heap_string_utf8 && filepath,
			engine::Hash strand,
			read_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new Read{id, directory, std::move(filepath), strand, callback, std::move(data), mode}; // todo

			const Message message{process_read, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}

		void remove_watch(
			system & system,
			engine::Token id)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new RemoveWatch{id}; // todo

			const Message message{process_remove_watch, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
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
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new Scan{id, directory, strand, callback, std::move(data), mode}; // todo

			const Message message{process_scan, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}

		void write(
			system & system,
			engine::Hash directory,
			ful::heap_string_utf8 && filepath,
			engine::Hash strand,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			if (!debug_assert(system->thread.valid()))
				return;

			auto * const ptr = new Write{directory, std::move(filepath), strand, callback, std::move(data), mode}; // todo

			const Message message{process_write, ptr};
			if (!debug_verify(ext::write_some_nonzero(system->pipe[1], &message, sizeof message) == sizeof message))
			{
				delete ptr;
			}
		}
	}
}

#endif
