#include "config.h"

#if FILE_SYSTEM_USE_POSIX

#include "system.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"
#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/system.hpp"
#include "engine/file/system/watch_posix.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"
#include "utility/ext/unistd.hpp"
#include "utility/shared_ptr.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
	struct Directory
	{
		utility::heap_string_utf8 dirpath;

		ext::ssize share_count;

		explicit Directory(utility::heap_string_utf8 && dirpath)
			: dirpath(std::move(dirpath))
			, share_count(0)
		{}
	};

	struct TemporaryDirectory
	{
		utility::heap_string_utf8 dirpath;

		explicit TemporaryDirectory(utility::heap_string_utf8 && dirpath)
			: dirpath(std::move(dirpath))
		{}
	};

	struct Alias
	{
		engine::Asset directory;
	};
}

namespace engine
{
	namespace file
	{
		struct system_impl
		{
			engine::task::scheduler * taskscheduler;

			engine::Asset strand;

			core::container::Collection
			<
				engine::Asset,
				utility::heap_storage_traits,
				utility::heap_storage<Directory>,
				utility::heap_storage<TemporaryDirectory>
			>
			directories;

			core::container::Collection
			<
				engine::Asset,
				utility::heap_storage_traits,
				utility::heap_storage<Alias>
			>
			aliases;

			const utility::heap_string_utf8 & get_dirpath(decltype(directories)::const_iterator it)
			{
				return directories.call(it, [](const auto & x) -> const utility::heap_string_utf8 & { return x.dirpath; });
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

		engine::file::initialize_watch();

		singelton.emplace();

		return &singelton.value();
	}

	void destroy_impl(engine::file::system_impl & /*impl*/)
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		singelton.reset();

		engine::file::deinitialize_watch();
	}
}

namespace
{
	void scan_directory(const utility::heap_string_utf8 & dirpath, bool recurse, utility::heap_string_utf8 & files)
	{
		utility::heap_vector<utility::heap_string_utf8> subdirs;
		if (!debug_verify(subdirs.try_emplace_back()))
			return;

		utility::heap_string_utf8 pattern;
		if (!debug_verify(pattern.try_append(dirpath)))
			return; // error

		while (!ext::empty(subdirs))
		{
			auto subdir = ext::back(std::move(subdirs));
			ext::pop_back(subdirs);

			if (!debug_verify(pattern.try_append(subdir)))
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

						if (!debug_verify(dir.try_append(subdir)))
							return; // error

						if (!debug_verify(dir.try_append(const_cast<const char *>(entry->d_name))))
							return; // error

						if (!debug_verify(dir.try_push_back('/')))
							return; // error
					}
				}
				else
				{
					if (!debug_verify(files.try_append(subdir)))
						return; // error

					if (!debug_verify(files.try_append(const_cast<const char *>(entry->d_name))))
						return; // error

					if (!debug_verify(files.try_push_back(';')))
						return; // error
				}
			}

			debug_verify(errno == 0, "readdir failed with errno ", errno);

			debug_verify(::closedir(dir) != -1, "failed with errno ", errno);

			pattern.reduce(subdir.size());
		}

		if (!empty(files))
		{
			files.reduce(1); // trailing ;
		}
	}

	bool read_file(engine::file::system_impl & impl, utility::heap_string_utf8 & filepath, std::uint32_t root, engine::file::read_callback * callback, utility::any & data)
	{
		const int fd = ::open(filepath.data(), O_RDONLY);
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

		utility::heap_string_utf8 relpath;
		if (debug_verify(relpath.try_append(filepath.data() + root, filepath.size() - root)))
		{
			core::ReadStream stream(
				[](void * dest, ext::usize n, void * data)
				{
					const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

					return ext::read_some_nonzero(fd, dest, n);
				},
				reinterpret_cast<void *>(fd),
				std::move(relpath));

			engine::file::system filesystem(impl);
			callback(filesystem, std::move(stream), data);
			filesystem.detach();
		}
		debug_verify(::close(fd) != -1, "failed with errno ", errno);
		return true;
	}

	void purge_temporary_directory(const utility::heap_string_utf8 & filepath)
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

	utility::const_string_iterator<utility::boundary_unit_utf8> check_filepath(utility::const_string_iterator<utility::boundary_unit_utf8> begin, utility::const_string_iterator<utility::boundary_unit_utf8> end)
	{
		// todo disallow windows absolute paths

		// todo report \\ as error

		while (true)
		{
			const auto slash = find(begin, end, '/');
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

	bool validate_filepath(utility::string_units_utf8 str)
	{
		return check_filepath(str.begin(), str.end()) == str.end();
	}

	bool add_file(utility::heap_string_utf8 & files, utility::string_units_utf8 subdir, utility::string_units_utf8 filename)
	{
		auto begin = files.begin();
		const auto end = files.end();
		if (begin != end)
		{
			while (true)
			{
				const auto split = find(begin, end, ';');
				if (utility::starts_with(begin, end, subdir))
				{
					const auto dir_skip = begin + subdir.size();
					if (utility::string_units_utf8(dir_skip, split) == filename)
						return false; // already added
				}

				if (split == end)
					break;

				begin = split + 1; // skip ;
			}

			if (!debug_verify(files.try_push_back(';')))
				return false;
		}

		if (!(debug_verify(files.try_append(subdir)) &&
		      debug_verify(files.try_append(filename))))
		{
			files.erase(begin == end ? files.begin() : rfind(files, ';'), files.end());
			return false;
		}

		return true;
	}

	bool remove_file(utility::heap_string_utf8 & files, utility::string_units_utf8 subdir, utility::string_units_utf8 filename)
	{
		auto begin = files.begin();
		const auto end = files.end();

		auto prev_split = begin;
		while (true)
		{
			const auto split = find(begin, end, ';');
			if (split == end)
				break;

			const auto next_begin = split + 1; // skip ;

			if (starts_with(begin, split, subdir) && utility::string_units_utf8(begin + subdir.size(), split) == filename)
			{
				files.erase(begin, next_begin);

				return true;
			}

			prev_split = split;
			begin = next_begin;
		}

		if (starts_with(begin, end, subdir) && utility::string_units_utf8(begin + subdir.size(), end) == filename)
		{
			files.erase(prev_split, end);

			return true;
		}

		return false; // already removed
	}

	void remove_duplicates(utility::heap_string_utf8 & files, utility::string_units_utf8 duplicates)
	{
		auto duplicates_begin = duplicates.begin();
		const auto duplicates_end = duplicates.end();

		if (duplicates_begin != duplicates_end)
		{
			while (true)
			{
				const auto duplicates_split = find(duplicates_begin, duplicates_end, ';');
				if (!debug_assert(duplicates_split != duplicates_begin, "unexpected file without name"))
					return; // error

				remove_file(files, L"", utility::string_units_utf8(duplicates_begin, duplicates_split));

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
			engine::Asset strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<FileMissingWork>()))
					{
						FileMissingWork && work = utility::any_cast<FileMissingWork &&>(std::move(data));
						engine::file::ReadData & read_data = *work.ptr;

						utility::heap_string_utf8 relpath;
						if (!debug_verify(relpath.try_append(read_data.filepath.data() + read_data.root, read_data.filepath.size() - read_data.root)))
							return; // error

						core::ReadStream stream(std::move(relpath));

						engine::file::system filesystem(read_data.impl);
						read_data.callback(filesystem, std::move(stream), read_data.data);
						filesystem.detach();
					}
				},
				std::move(data));
		}

		void post_work(FileReadWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Asset strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
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
							utility::heap_string_utf8 relpath;
							if (!debug_verify(relpath.try_append(read_data.filepath.data() + read_data.root, read_data.filepath.size() - read_data.root)))
								return; // error

							core::ReadStream stream(std::move(relpath));

							engine::file::system filesystem(read_data.impl);
							read_data.callback(filesystem, std::move(stream), read_data.data);
							filesystem.detach();
						}
					}
				},
				std::move(data));
		}

		void post_work(ScanChangeWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Asset strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<ScanChangeWork>()))
					{
						ScanChangeWork && work = utility::any_cast<ScanChangeWork &&>(std::move(data));
						engine::file::ScanData & scan_data = *work.ptr;

						utility::heap_string_utf8 old_files = scan_data.files;
						// todo garbage callback if copy fails

						utility::string_units_utf8 subdir(work.filepath.begin() + scan_data.dirpath.size(), work.filepath.end());

						for (const auto & file : work.files)
						{
							switch (front(file))
							{
							case '+':
								add_file(scan_data.files, subdir, utility::string_units_utf8(file.begin() + 1, file.end()));
								break;
							case '-':
								remove_file(scan_data.files, subdir, utility::string_units_utf8(file.begin() + 1, file.end()));
								break;
							default:
								debug_unreachable("unknown file change");
							}
						}

						remove_duplicates(old_files, scan_data.files);

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, utility::heap_string_utf8(scan_data.files), std::move(old_files), scan_data.data);
						filesystem.detach();
					}
				},
				std::move(data));
		}

		void post_work(ScanOnceWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Asset strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<ScanOnceWork>()))
					{
						ScanOnceWork && work = utility::any_cast<ScanOnceWork &&>(std::move(data));
						engine::file::ScanData & scan_data = *work.ptr;

						utility::heap_string_utf8 removed_files = std::move(scan_data.files);
						scan_directory(scan_data.dirpath, false, scan_data.files);
						remove_duplicates(removed_files, scan_data.files);

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, utility::heap_string_utf8(scan_data.files), std::move(removed_files), scan_data.data);
						filesystem.detach();
					}
				},
				std::move(data));
		}

		void post_work(ScanRecursiveWork && data)
		{
			engine::task::scheduler & taskscheduler = *data.ptr->impl.taskscheduler;
			engine::Asset strand = data.ptr->strand;

			engine::task::post_work(
				taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<ScanRecursiveWork>()))
					{
						ScanRecursiveWork && work = utility::any_cast<ScanRecursiveWork &&>(std::move(data));
						engine::file::ScanData & scan_data = *work.ptr;

						utility::heap_string_utf8 removed_files = std::move(scan_data.files);
						scan_directory(scan_data.dirpath, true, scan_data.files);
						remove_duplicates(removed_files, scan_data.files);

						engine::file::system filesystem(scan_data.impl);
						scan_data.callback(filesystem, scan_data.directory, utility::heap_string_utf8(scan_data.files), std::move(removed_files), scan_data.data);
						filesystem.detach();
					}
				},
				std::move(data));
		}
	}
}

namespace
{
	struct RegisterDirectory
	{
		engine::file::system_impl & impl;
		engine::Asset alias;
		utility::heap_string_utf8 filepath;
		engine::Asset parent;
	};

	struct RegisterTemporaryDirectory
	{
		engine::file::system_impl & impl;
		engine::Asset alias;
	};

	struct UnregisterDirectory
	{
		engine::file::system_impl & impl;
		engine::Asset alias;
	};

	struct Read
	{
		engine::file::system_impl & impl;
		engine::Identity id;
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
		engine::Asset strand;
		engine::file::read_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct RemoveWatch
	{
		engine::file::system_impl & impl;
		engine::Identity id;
	};

	struct Scan
	{
		engine::file::system_impl & impl;
		engine::Identity id;
		engine::Asset directory;
		engine::Asset strand;
		engine::file::scan_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct Write
	{
		engine::file::system_impl & impl;
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
		engine::Asset strand;
		engine::file::write_callback * callback;
		utility::any data;
		engine::file::flags mode;
	};

	struct Terminate
	{
		engine::file::system_impl & impl;
		core::sync::Event<true> & event;
	};

	void process_register_directory(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<RegisterDirectory>()))
			return;

		auto && x = utility::any_cast<RegisterDirectory &&>(std::move(data));

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

		utility::heap_string_utf8 filepath;
		if (!debug_verify(filepath.try_append(dirpath)))
			return; // error

		if (!debug_verify(filepath.try_append(x.filepath)))
			return; // error

		if (back(filepath) != '/')
		{
			if (!debug_verify(filepath.try_push_back('/')))
				return; // error
		}

		const auto directory_asset = engine::Asset(filepath);
		const auto directory_it = find(x.impl.directories, directory_asset);
		if (directory_it != x.impl.directories.end())
		{
			const auto directory_ptr = x.impl.directories.get<Directory>(directory_it);
			if (!debug_verify(directory_ptr))
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

	void process_register_temporary_directory(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<RegisterTemporaryDirectory>()))
			return;

		auto && x = utility::any_cast<RegisterTemporaryDirectory &&>(std::move(data));

		if (!debug_verify(find(x.impl.aliases, x.alias) == x.impl.aliases.end()))
			return; // error

		utility::heap_string_utf8 filepath(u8"" P_tmpdir "/unnamed-XXXXXX"); // todo project name
		if (!debug_verify(::mkdtemp(filepath.data()) != nullptr, "failed with errno ", errno))
			return; // error

		debug_printline("created temporary directory \"", filepath, "\"");

		if (!debug_verify(filepath.try_append('/')))
		{
			purge_temporary_directory(filepath);
			return; // error
		}

		const auto directory_asset = engine::Asset(filepath);
		if (!debug_verify(x.impl.directories.emplace<TemporaryDirectory>(directory_asset, std::move(filepath))))
		{
			purge_temporary_directory(filepath);
			return; // error
		}

		if (!debug_verify(x.impl.aliases.emplace<Alias>(x.alias, directory_asset)))
			return; // error
	}

	void process_unregister_directory(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<UnregisterDirectory>()))
			return;

		auto && x = utility::any_cast<UnregisterDirectory &&>(std::move(data));

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
		} detach_alias;

		const auto directory_is_unused = x.impl.directories.call(directory_it, detach_alias);
		if (directory_is_unused)
		{
			x.impl.directories.erase(directory_it);
		}

		x.impl.aliases.erase(alias_it);
	}

	void process_read(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Read>()))
			return;

		auto && x = utility::any_cast<Read &&>(std::move(data));

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

		utility::heap_string_utf8 filepath;
		if (!debug_verify(filepath.try_append(dirpath)))
			return; // error

		if (!debug_verify(filepath.try_append(x.filepath)))
			return; // error

		ext::heap_shared_ptr<engine::file::ReadData> data_ptr(utility::in_place, x.impl, std::move(filepath), static_cast<std::uint32_t>(dirpath.size()), x.strand, x.callback, std::move(x.data));
		if (!debug_verify(data_ptr))
			return; // error

		if (x.mode & engine::file::flags::ADD_WATCH)
		{
			engine::file::add_file_watch(x.id, data_ptr, static_cast<bool>(x.mode & engine::file::flags::REPORT_MISSING));
		}

		engine::file::post_work(engine::file::FileReadWork{std::move(data_ptr)});
	}

	void process_remove_watch(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<RemoveWatch>()))
			return;

		auto && x = utility::any_cast<RemoveWatch &&>(std::move(data));

		engine::file::remove_watch(x.id);
	}

	void process_scan(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Scan>()))
			return;

		auto && x = utility::any_cast<Scan &&>(std::move(data));

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

		ext::heap_shared_ptr<engine::file::ScanData> call_ptr(utility::in_place, x.impl, dirpath, x.directory, x.strand, x.callback, std::move(x.data), utility::heap_string_utf8());
		if (!debug_verify(call_ptr))
			return; // error

		if (x.mode & engine::file::flags::ADD_WATCH)
		{
			engine::file::add_scan_watch(x.id, call_ptr, static_cast<bool>(x.mode & engine::file::flags::RECURSE_DIRECTORIES));
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

	void process_write(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Write>()))
			return;

		auto && x = utility::any_cast<Write &&>(std::move(data));

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

		utility::heap_string_utf8 filepath;
		if (!debug_verify(filepath.try_append(dirpath)))
			return; // error

		if (!debug_verify(filepath.try_append(x.filepath)))
			return; // error

		if (x.mode & engine::file::flags::CREATE_DIRECTORIES)
		{
			auto begin = filepath.begin() + dirpath.size();
			const auto end = filepath.end();

			while (true)
			{
				const auto slash = find(begin, end, '/');
				if (slash == end)
					break;

				*slash = '\0';

				if (::mkdir(filepath.data(), 0775) != 0)
				{
					if (!debug_verify(errno == EEXIST))
						return; // error

					struct stat buf;
					if (!debug_verify(stat(filepath.data(), &buf) == 0))
						return; // error

					if (!debug_verify(S_ISDIR(buf.st_mode)))
						return; // error
				}

				*slash = '/';

				begin = slash + 1;
			}
		}

		// todo define _FILE_OFFSET_BITS 64 (see open(2))

		int flags = O_WRONLY | O_CREAT | O_EXCL;
		int mode = 0664;
		if (x.mode & engine::file::flags::APPEND_EXISTING)
		{
			flags = O_WRONLY | O_APPEND;
		}
		else if (x.mode & engine::file::flags::OVERWRITE_EXISTING)
		{
			flags = O_WRONLY | O_CREAT | O_TRUNC;
		}

		const int fd = ::open(filepath.data(), flags, mode);
		if (fd == -1)
		{
			debug_verify(errno == EEXIST, "open \"", filepath, "\" failed with errno ", errno);

			return;
		}

		// todo can this lock be acquired on open?
		while (::flock(fd, LOCK_EX) != 0)
		{
			if (!debug_verify(errno == EINTR))
			{
				break;
			}
		}

		core::WriteStream stream(
			[](const void * src, ext::usize n, void * data)
			{
				const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

				return ext::write_some_nonzero(fd, src, n);
			},
			reinterpret_cast<void *>(fd),
			std::move(x.filepath));

		engine::file::system filesystem(x.impl);
		x.callback(filesystem, std::move(stream), std::move(x.data));
		filesystem.detach();

		debug_verify(::close(fd) != -1, "failed with errno ", errno);
	}

	void process_terminate(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Terminate>()))
			return;

		auto && x = utility::any_cast<Terminate &&>(std::move(data));

		x.event.set();
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

			const auto len = ext::strlen(malloc_str) + 1; // one extra for trailing /
			if (debug_verify(dir.filepath_.try_append(malloc_str, len))) // it is fine to copy null
			{
				dir.filepath_.data()[len - 1] = '/';
			}

			::free(malloc_str);

			return dir;
		}

		void system::destruct(system_impl & impl)
		{
			core::sync::Event<true> event;

			engine::task::post_work(*impl.taskscheduler, impl.strand, process_terminate, utility::any(utility::in_place_type<Terminate>, impl, event));

			event.wait();

			destroy_impl(impl);
		}

		system_impl * system::construct(engine::task::scheduler & taskscheduler, directory && root)
		{
			system_impl * const impl = create_impl();
			if (debug_verify(impl))
			{
				impl->taskscheduler = &taskscheduler;

				const auto root_asset = engine::Asset(root.filepath_);

				if (!debug_verify(impl->directories.emplace<Directory>(root_asset, std::move(root.filepath_))))
				{
					destroy_impl(*impl);
					return nullptr;
				}

				impl->strand = root_asset; // the strand ought to be something unique, this is probably good enough

				if (!debug_verify(impl->aliases.emplace<Alias>(engine::file::working_directory, root_asset)))
				{
					impl->directories.clear();
					destroy_impl(*impl);
					return nullptr;
				}
			}
			return impl;
		}

		void register_directory(system & system, engine::Asset name, utility::heap_string_utf8 && filepath, engine::Asset parent)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_register_directory, utility::any(utility::in_place_type<RegisterDirectory>, *system, name, std::move(filepath), parent));
		}

		void register_temporary_directory(system & system, engine::Asset name)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_register_temporary_directory, utility::any(utility::in_place_type<RegisterTemporaryDirectory>, *system, name));
		}

		void unregister_directory(system & system, engine::Asset name)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_unregister_directory, utility::any(utility::in_place_type<UnregisterDirectory>, *system, name));
		}

		void read(
			system & system,
			engine::Identity id,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			engine::Asset strand,
			read_callback * callback,
			utility::any && data,
			flags mode)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_read, utility::any(utility::in_place_type<Read>, *system, id, directory, std::move(filepath), strand, callback, std::move(data), mode));
		}

		void remove_watch(
			system & system,
			engine::Identity id)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_remove_watch, utility::any(utility::in_place_type<RemoveWatch>, *system, id));
		}

		void scan(
			system & system,
			engine::Identity id,
			engine::Asset directory,
			engine::Asset strand,
			scan_callback * callback,
			utility::any && data,
			flags mode)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_scan, utility::any(utility::in_place_type<Scan>, *system, id, directory, strand, callback, std::move(data), mode));
		}

		void write(
			system & system,
			engine::Asset directory,
			utility::heap_string_utf8 && filepath,
			engine::Asset strand,
			write_callback * callback,
			utility::any && data,
			flags mode)
		{
			engine::task::post_work(*system->taskscheduler, system->strand, process_write, utility::any(utility::in_place_type<Write>, *system, directory, std::move(filepath), strand, callback, std::move(data), mode));
		}
	}
}

#endif
