#include "engine/file/loader.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/file/system.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"
#include "utility/algorithm/find.hpp"
#include "utility/functional/utility.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/variant.hpp"
#include "utility/weak_ptr.hpp"

#include <atomic>
#include <memory>

namespace ext
{
	namespace detail
	{
		template <typename ...Ts>
		class overload_set;

		template <typename T1, typename T2>
		class overload_set<T1, T2>
			: T1
			, T2
		{
		public:
			template <typename P1, typename P2>
			explicit overload_set(P1 && p1, P2 && p2)
				: T1(std::forward<P1>(p1))
				, T2(std::forward<P2>(p2))
			{}

			using T1::operator ();
			using T2::operator ();
		};

		template <typename T1, typename ...Ts>
		class overload_set<T1, Ts...>
			: T1
			, overload_set<Ts...>
		{
		public:
			template <typename P1, typename ...Ps>
			explicit overload_set(P1 && p1, Ps && ...ps)
				: T1(std::forward<P1>(p1))
				, overload_set<Ts...>(std::forward<Ps>(ps)...)
			{}

			using T1::operator ();
			using overload_set<Ts...>::operator ();
		};
	}

	template <typename P>
	decltype(auto) overload(P && p)
	{
		return std::forward<P>(p);
	}

	template <typename ...Ps>
	decltype(auto) overload(Ps && ...ps)
	{
		return detail::overload_set<mpl::remove_cvref_t<Ps>...>(std::forward<Ps>(ps)...);
	}
}

namespace
{
	struct Filetype
	{
		struct Callback
		{
			engine::file::load_callback * loadcall;
			engine::file::unload_callback * unloadcall;
			utility::any data;
		};

		ext::heap_shared_ptr<Callback> callback;
	};

	struct FileCallData
	{
		struct Callback
		{
			engine::file::ready_callback * readycall;
			engine::file::unready_callback * unreadycall;
			utility::any data;
		};

		utility::heap_vector<engine::Asset, Callback> calls;

		bool ready = false;
	};

	struct FileCallDataPlusOne
	{
		ext::heap_shared_ptr<FileCallData> call_ptr;

		engine::Asset file_alias;
		FileCallData::Callback new_call;
	};

	struct ReadData
	{
		engine::Asset file;
		ext::heap_weak_ptr<Filetype::Callback> filetype_callback;
		ext::heap_weak_ptr<FileCallData> file_callback;

		static void file_load(core::ReadStream && stream, utility::any & data);
	};

	constexpr auto global = engine::Asset{};
	constexpr auto strand = engine::Asset("_file_loader_");
}

namespace
{
	struct MessageFileScan
	{
		engine::Asset directory;
		utility::heap_string_utf8 files;
	};

	struct MessageLoadGlobal
	{
		engine::Asset filetype;
		engine::Asset file;
		engine::file::ready_callback * readycall;
		engine::file::unready_callback * unreadycall;
		utility::any data;
	};

	struct MessageLoadLocal
	{
		engine::Asset filetype;
		engine::Asset owner;
		engine::Asset file;
		engine::file::ready_callback * readycall;
		engine::file::unready_callback * unreadycall;
		utility::any data;
	};

	struct MessageLoadDependency
	{
		engine::Asset filetype;
		engine::Asset owner;
		engine::Asset file;
		engine::file::ready_callback * readycall;
		engine::file::unready_callback * unreadycall;
		utility::any data;
	};

	struct MessageLoadDone
	{
		engine::Asset file;
	};

	struct MessageLoadInit
	{
		engine::Asset file;
	};

	struct MessageRegisterFiletype
	{
		engine::Asset filetype;
		engine::file::load_callback * loadcall;
		engine::file::unload_callback * unloadcall;
		utility::any data;
	};

	struct MessageRegisterLibrary
	{
		engine::Asset directory;
	};

	struct MessageUnloadGlobal
	{
		engine::Asset file;
	};

	struct MessageUnloadLocal
	{
		engine::Asset owner;
		engine::Asset file;
	};

	struct MessageUnloadDependency
	{
		engine::Asset owner;
		engine::Asset file;
	};

	struct MessageUnregisterFiletype
	{
		engine::Asset filetype;
	};

	struct MessageUnregisterLibrary
	{
		engine::Asset directory;
	};

	using Message = utility::variant
	<
		MessageFileScan,
		MessageLoadGlobal,
		MessageLoadLocal,
		MessageLoadDependency,
		MessageLoadDone,
		MessageLoadInit,
		MessageRegisterFiletype,
		MessageRegisterLibrary,
		MessageUnloadGlobal,
		MessageUnloadLocal,
		MessageUnloadDependency,
		MessageUnregisterFiletype,
		MessageUnregisterLibrary
	>;
}

namespace
{
	engine::task::scheduler * module_taskscheduler = nullptr;
	engine::file::system * module_filesystem = nullptr;

	using FileCallPtr = ext::heap_shared_ptr<FileCallData>;

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<Filetype>
	>
	filetypes;

	struct AmbiguousFile
	{
		utility::heap_vector<engine::Asset> files;
	};

	struct DirectoryFile
	{
	};

	struct KnownFile
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;
	};

	struct LoadingFile
	{
		engine::Asset filetype;

		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		FileCallPtr call_ptr;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		std::int32_t previous_count; // the number of attachments after loading completed
		std::int32_t remaining_count; // the remaining number of attachments that are required (the most significant bit is set if the file itself is not yet finished reading)

		explicit LoadingFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, call_ptr(std::move(call_ptr))
			, previous_count(-1)
			, remaining_count(INT_MIN)
		{}

		explicit LoadingFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, call_ptr(std::move(call_ptr))
			, owners(std::move(owners))
			, attachments(std::move(attachments))
			, previous_count(static_cast<int32_t>(this->attachments.size()))
			, remaining_count(INT_MIN)
		{}
	};

	struct LoadedFile
	{
		engine::Asset filetype;

		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		FileCallPtr call_ptr;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		explicit LoadedFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, call_ptr(std::move(call_ptr))
			, owners(std::move(owners))
			, attachments(std::move(attachments))
		{}
	};

	struct UniqueFile
	{
		engine::Asset file;
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<AmbiguousFile>,
		utility::heap_storage<DirectoryFile>,
		utility::heap_storage<KnownFile>,
		utility::heap_storage<LoadingFile>,
		utility::heap_storage<LoadedFile>,
		utility::heap_storage<UniqueFile>
	>
	files;

	std::pair<engine::Asset, decltype(files.end())> find_underlying_file(engine::Asset file)
	{
		const auto file_it = find(files, file);
		if (file_it != files.end())
		{
			if (const UniqueFile * const radical = files.get<UniqueFile>(file_it))
				return std::make_pair(radical->file, find(files, radical->file));
		}
		return std::make_pair(file, file_it);
	}

	bool make_known(engine::Asset file, decltype(files.end()) file_it, engine::Asset directory, utility::heap_string_utf8 && filepath)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		return debug_verify(files.emplace<KnownFile>(file, directory, std::move(filepath)));
	}

	bool make_loaded(engine::Asset file, decltype(files.end()) file_it, LoadingFile tmp)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		const auto loaded_file = files.emplace<LoadedFile>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.call_ptr), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loaded_file))
			return false;

		engine::task::post_work(
			*module_taskscheduler,
			file,
			[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
		{
			if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
			{
				FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
				FileCallData & call_data = *call_ptr;

				if (debug_assert(!call_data.ready))
				{
					for (auto && call : call_data.calls)
					{
						call.second.readycall(call.second.data, call.first);
					}
					call_data.ready = true;
				}
			}
		},
			loaded_file->call_ptr);

		return true;
	}

	bool make_loading(engine::Asset file, decltype(files.end()) file_it, LoadedFile tmp)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		const auto loading_file = files.emplace<LoadingFile>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.call_ptr), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loading_file))
			return false;

		return true;
	}

	void remove_attachments(utility::heap_vector<engine::Asset, engine::Asset> && relations)
	{
		while (!ext::empty(relations))
		{
			const utility::heap_vector<engine::Asset, engine::Asset>::value_type relation = ext::back(std::move(relations));
			ext::pop_back(relations);

			const auto file_it = find(files, relation.second);
			if (!debug_assert(file_it != files.end()))
				break;

			files.call(file_it, ext::overload(
				[&](AmbiguousFile &)
			{
				debug_unreachable();
			},
				[&](DirectoryFile &)
			{
				debug_unreachable();
			},
				[&](KnownFile &)
			{
				debug_unreachable();
			},
				[&](LoadingFile & x)
			{
				const auto owner_it = ext::find(x.owners, relation.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					const auto mode = engine::file::flags::ADD_WATCH;
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath), mode);
#endif

					engine::task::post_work(
						*module_taskscheduler,
						relation.second,
						[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
					{
						if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
						{
							FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
							FileCallData & call_data = *call_ptr;

							if (debug_assert(call_data.ready))
							{
								for (auto && call : call_data.calls)
								{
									call.second.unreadycall(call.second.data, call.first);
								}
								call_data.ready = false;
							}
						}
					},
						x.call_ptr);
					// note purposfully not call unload (since it has not been loaded)

					if (debug_verify(relations.try_reserve(relations.size() + x.attachments.size())))
					{
						for (auto attachment : x.attachments)
						{
							relations.try_emplace_back(utility::no_failure, relation.second, attachment);
						}
					}

					const auto directory = x.directory;
					const auto filepath = std::move(x.filepath);

					files.erase(file_it);
					debug_verify(files.emplace<KnownFile>(relation.second, directory, std::move(filepath)));
				}
			},
				[&](LoadedFile & x)
			{
				const auto owner_it = ext::find(x.owners, relation.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					const auto mode = engine::file::flags::ADD_WATCH;
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath), mode);
#endif

					engine::task::post_work(
						*module_taskscheduler,
						relation.second,
						[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
					{
						if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
						{
							FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
							FileCallData & call_data = *call_ptr;

							if (debug_assert(call_data.ready))
							{
								for (auto && call : call_data.calls)
								{
									call.second.unreadycall(call.second.data, call.first);
								}
								call_data.ready = false;
							}
						}
					},
						x.call_ptr);
					const auto filetype_it = find(filetypes, x.filetype);
					if (debug_assert(filetype_it != filetypes.end()))
					{
						Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
						if (debug_assert(filetype_ptr))
						{
							engine::task::post_work(
								*module_taskscheduler,
								relation.second,
								[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
							{
								if (debug_assert(data.type_id() == (utility::type_id<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset>>())))
								{
									std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> call_ptr = utility::any_cast<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> &&>(std::move(data));
									Filetype::Callback & call_data = *call_ptr.first;

									call_data.unloadcall(call_data.data, call_ptr.second);
								}
							},
								std::make_pair(filetype_ptr->callback, relation.second));
						}
					}

					if (debug_verify(relations.try_reserve(relations.size() + x.attachments.size())))
					{
						for (auto attachment : x.attachments)
						{
							relations.try_emplace_back(utility::no_failure, relation.second, attachment);
						}
					}

					const auto directory = x.directory;
					const auto filepath = std::move(x.filepath);

					files.erase(file_it);
					debug_verify(files.emplace<KnownFile>(relation.second, directory, std::move(filepath)));
				}
			},
				[&](UniqueFile &)
			{
				debug_unreachable();
			}));
		}
	}

	bool load_file(
		engine::Asset owner,
		engine::Asset filetype,
		engine::Asset file,
		engine::Asset actual_file,
		decltype(files.end()) file_it,
		engine::file::ready_callback * readycall,
		engine::file::unready_callback * unreadycall,
		utility::any && data)
	{
		return files.call(file_it, ext::overload(
			[](AmbiguousFile &)
		{
			return debug_fail("cannot load ambiguous files");
		},
			[](DirectoryFile &)
		{
			return debug_fail("cannot load directory files");
		},
			[&](KnownFile & y)
		{
			const auto filetype_it = find(filetypes, filetype);
			if (!debug_verify(filetype_it != filetypes.end()))
				return false; // error

			KnownFile copy = std::move(y);

			FileCallPtr call_ptr(utility::in_place);
			if (!debug_verify(call_ptr->calls.try_emplace_back(actual_file, FileCallData::Callback{readycall, unreadycall, std::move(data)})))
				return false; // error

			// todo replace
			files.erase(file_it);
			// todo on failure the file is lost
			LoadingFile * const loading_file = files.emplace<LoadingFile>(file, filetype, copy.directory, std::move(copy.filepath), std::move(call_ptr));
			if (!debug_verify(loading_file))
				return false; // error

			if (!debug_verify(loading_file->owners.try_emplace_back(owner)))
			{
				files.erase(find(files, file));
				return false; // error
			}

			Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
			if (!debug_assert(filetype_ptr))
			{
				files.erase(find(files, file));
				return false;
			}

#if MODE_DEBUG
			const auto mode = engine::file::flags::ADD_WATCH; // todo ought to add engine::file::flags::RECURSE_DIRECTORIES if filepath contains subdir, but since a recursive scan watch has already been made it might be fine anyway?
#else
			const auto mode = engine::file::flags{};
#endif
			engine::file::read(*::module_filesystem, loading_file->directory, utility::heap_string_utf8(loading_file->filepath), file, ReadData::file_load, ReadData{file, ext::heap_weak_ptr<Filetype::Callback>(filetype_ptr->callback), ext::heap_weak_ptr<FileCallData>(loading_file->call_ptr)/*, std::weak_ptr<FileCallback>(loading_file->callback)*/}, mode);

			return true;
		},
			[&](LoadingFile & y)
		{
			if (!debug_assert(y.filetype == filetype))
				return false;

			if (!debug_verify(y.call_ptr->calls.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(actual_file), std::forward_as_tuple(readycall, unreadycall, std::move(data)))))
				return false;

			if (!debug_verify(y.owners.try_emplace_back(owner)))
			{
				ext::pop_back(y.call_ptr->calls);
				return false;
			}

			return true;
		},
			[&](LoadedFile & y)
		{
			if (!debug_assert(y.filetype == filetype))
				return false;

			if (!debug_verify(y.owners.try_emplace_back(owner)))
				return false; // error

			engine::task::post_work(
				*module_taskscheduler,
				file,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
			{
				if (debug_assert(data.type_id() == utility::type_id<FileCallDataPlusOne>()))
				{
					FileCallDataPlusOne call_ptr = utility::any_cast<FileCallDataPlusOne &&>(std::move(data));
					FileCallData & call_data = *call_ptr.call_ptr;

					if (!debug_verify(call_data.calls.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(call_ptr.file_alias), std::forward_as_tuple(call_ptr.new_call.readycall, call_ptr.new_call.unreadycall, std::move(call_ptr.new_call.data)))))
						return;

					if (debug_assert(call_data.ready))
					{
						auto && call = ext::back(call_data.calls);

						call.second.readycall(call.second.data, call.first);
					}
				}
			},
				FileCallDataPlusOne{y.call_ptr, actual_file, FileCallData::Callback{readycall, unreadycall, std::move(data)}});

			return true;
		},
			[&](UniqueFile &) -> bool
		{
			debug_unreachable("radicals should not point to other radicals");
		}));
	}

	bool remove_file(
		engine::Asset file,
		decltype(files.end()) file_it)
	{
		return files.call(file_it, ext::overload(
			[&](AmbiguousFile &) -> bool
		{
			debug_unreachable("file ", file, " is ambiguous");
		},
			[&](DirectoryFile &)
		{
			return debug_fail("file ", file, " is a directory and cannot be unloaded");
		},
			[&](KnownFile &)
		{
			return debug_fail("cannot unload already unloaded file ", file);
		},
			[&](LoadingFile & y)
		{
			const auto owner_it = ext::find(y.owners, global);
			if (!debug_verify(owner_it != y.owners.end(), "file ", file, " is not global"))
				return false; // error

			y.owners.erase(owner_it);

			if (ext::empty(y.owners))
			{
#if MODE_DEBUG
				const auto mode = engine::file::flags::ADD_WATCH;
				engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath), mode);
#endif

				if (0 <= y.previous_count)
				{
					const auto filetype_it = find(filetypes, y.filetype);
					if (debug_assert(filetype_it != filetypes.end()))
					{
						Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
						if (debug_assert(filetype_ptr))
						{
							engine::task::post_work(
								*module_taskscheduler,
								file,
								[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
							{
								if (debug_assert(data.type_id() == (utility::type_id<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset>>())))
								{
									std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> call_ptr = utility::any_cast<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> &&>(std::move(data));
									Filetype::Callback & call_data = *call_ptr.first;

									call_data.unloadcall(call_data.data, call_ptr.second);
								}
							},
								std::make_pair(filetype_ptr->callback, file));
						}
					}
				}

				utility::heap_vector<engine::Asset, engine::Asset> relations;
				if (debug_verify(relations.try_reserve(y.attachments.size())))
				{
					for (auto attachment : y.attachments)
					{
						relations.try_emplace_back(utility::no_failure, file, attachment);
					}
				}
				remove_attachments(std::move(relations));
			}
			return true;
		},
			[&](LoadedFile & y)
		{
			const auto owner_it = ext::find(y.owners, global);
			if (!debug_verify(owner_it != y.owners.end(), "file ", file, " is not global"))
				return false; // error

			y.owners.erase(owner_it);

			if (ext::empty(y.owners))
			{
#if MODE_DEBUG
				const auto mode = engine::file::flags::ADD_WATCH;
				engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath), mode);
#endif

				engine::task::post_work(
					*module_taskscheduler,
					file,
					[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
				{
					if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
					{
						FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
						FileCallData & call_data = *call_ptr;

						if (debug_assert(call_data.ready))
						{
							for (auto && call : call_data.calls)
							{
								call.second.unreadycall(call.second.data, call.first);
							}
							call_data.ready = false;
						}
					}
				},
					y.call_ptr);
				const auto filetype_it = find(filetypes, y.filetype);
				if (debug_assert(filetype_it != filetypes.end()))
				{
					Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
					if (debug_assert(filetype_ptr))
					{
						engine::task::post_work(
							*module_taskscheduler,
							file,
							[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
						{
							if (debug_assert(data.type_id() == (utility::type_id<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset>>())))
							{
								std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> call_ptr = utility::any_cast<std::pair<ext::heap_shared_ptr<Filetype::Callback>, engine::Asset> &&>(std::move(data));
								Filetype::Callback & call_data = *call_ptr.first;

								call_data.unloadcall(call_data.data, call_ptr.second);
							}
						},
							std::make_pair(filetype_ptr->callback, file));
					}
				}

				utility::heap_vector<engine::Asset, engine::Asset> relations;
				if (debug_verify(relations.try_reserve(y.attachments.size())))
				{
					for (auto attachment : y.attachments)
					{
						relations.try_emplace_back(utility::no_failure, file, attachment);
					}
				}
				remove_attachments(std::move(relations));
			}
			return true;
		},
			[&](UniqueFile &) -> bool
		{
			debug_unreachable("radicals should not point to other radicals");
		}));
	}

	void finish_loading(engine::Asset file, decltype(files.end()) file_it, LoadingFile && loading_file)
	{
		utility::heap_vector<engine::Asset, engine::Asset> relations;
		if (debug_verify(relations.try_reserve(loading_file.owners.size())))
		{
			for (auto owner : loading_file.owners)
			{
				relations.try_emplace_back(utility::no_failure, owner, file);
			}
		}

		if (0 < loading_file.previous_count)
		{
			utility::heap_vector<engine::Asset, engine::Asset> relations_;
			if (debug_verify(relations_.try_reserve(loading_file.previous_count)))
			{
				const auto split_it = loading_file.attachments.begin() + loading_file.previous_count;
				for (auto attachment_it = loading_file.attachments.begin(); attachment_it != split_it; ++attachment_it)
				{
					relations_.try_emplace_back(utility::no_failure, file, *attachment_it);
				}
				loading_file.attachments.erase(loading_file.attachments.begin(), split_it);
			}
			remove_attachments(std::move(relations_));
		}

		if (!make_loaded(file, file_it, std::move(loading_file)))
			return; // error

		while (!ext::empty(relations))
		{
			const utility::heap_vector<engine::Asset, engine::Asset>::value_type relation = ext::back(std::move(relations));
			ext::pop_back(relations);

			const auto owner_it = find(files, relation.first);
			if (!debug_assert(file_it != files.end()))
				break;

			if (owner_it == files.end())
			{
				if (debug_assert(relation.first == global))
					continue;

				return;
			}

			if (LoadingFile * const loading_owner = files.get<LoadingFile>(owner_it))
			{
				const auto attachment_it = ext::find(loading_owner->attachments, relation.second);
				if (debug_assert(attachment_it != loading_owner->attachments.end()))
				{
					const auto count = loading_owner->remaining_count & INT_MAX;
					if (loading_owner->attachments.end() - attachment_it <= count)
					{
						debug_assert(0 < count);

						const auto split_it = loading_owner->attachments.end() - count;
						std::swap(*attachment_it, *split_it);

						loading_owner->remaining_count--;
						if (loading_owner->remaining_count == 0)
						{
							if (debug_verify(relations.try_reserve(relations.size() + loading_owner->owners.size())))
							{
								for (auto owner : loading_owner->owners)
								{
									relations.try_emplace_back(utility::no_failure, owner, relation.first);
								}
							}

							if (0 < loading_owner->previous_count)
							{
								utility::heap_vector<engine::Asset, engine::Asset> relations_;
								if (debug_verify(relations_.try_reserve(loading_owner->previous_count)))
								{
									const auto split_it_ = loading_owner->attachments.begin() + loading_owner->previous_count;
									for (auto attachment_it_ = loading_owner->attachments.begin(); attachment_it_ != split_it_; ++attachment_it_)
									{
										relations_.try_emplace_back(utility::no_failure, relation.second, *attachment_it_);
									}
									loading_owner->attachments.erase(loading_owner->attachments.begin(), split_it_);
								}
								remove_attachments(std::move(relations_));
							}

							if (!make_loaded(relation.first, owner_it, std::move(*loading_owner)))
								return; // error
						}
					}
				}
			}
		}
	}

	engine::Asset scanning_directory{};
	utility::heap_vector<Message> delayed_messages;

	void process_message(Message && message)
	{
		struct ProcessMessage
		{
			void operator () (MessageFileScan && x)
			{
				scanning_directory = engine::Asset{};

				utility::heap_vector<engine::Asset> keys;
				if (!debug_verify(keys.try_reserve(files.table_size())))
					return; // error

				for (auto _ : ranges::index_sequence(files.table_size()))
				{
					static_cast<void>(_);
					keys.try_emplace_back(utility::no_failure);
				}

				const auto nkeys = files.get_all_keys(keys.data(), keys.size()) - keys.data();
				keys.erase(keys.begin() + nkeys, keys.end());

				auto begin = x.files.begin();
				const auto end = x.files.end();

				if (begin != end)
				{
					while (true)
					{
						const auto split = find(begin, end, u8';');
						if (!debug_assert(split != begin, "unexpected file without name"))
							return; // error

						const auto filepath = utility::string_units_utf8(begin, split);
						const auto file_asset = engine::Asset(filepath);

						if (ext::contains(keys, file_asset))
						{
							const auto file_it = find(files, file_asset);
							if (debug_verify(file_it != files.end()))
							{
								files.call(file_it, ext::overload(
									[&](AmbiguousFile &)
								{
									if (!debug_assert(find(begin, split, u8'.') == split))
										return;

									files.erase(file_it);

									if (!debug_verify(files.emplace<KnownFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
										return; // error
								},
									[&](DirectoryFile &)
								{
									debug_unreachable(file_asset, " has previously been registered as a directory");
								},
									[&](KnownFile & y)
								{
									debug_assert(y.directory == x.directory) && debug_assert(y.filepath == filepath);
								},
									[&](LoadingFile & y)
								{
									debug_assert(y.directory == x.directory) && debug_assert(y.filepath == filepath);
								},
									[&](LoadedFile & y)
								{
									debug_assert(y.directory == x.directory) && debug_assert(y.filepath == filepath);
								},
									[&](UniqueFile &) // todo same as AmbiguousFile
								{
									if (!debug_assert(find(begin, split, u8'.') == split))
										return;

									files.erase(file_it);

									if (!debug_verify(files.emplace<KnownFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
										return; // error
								}));
							}
						}
						else
						{
							if (debug_verify(find(files, file_asset) == files.end()))
							{
								if (!debug_verify(files.emplace<KnownFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
									return; // error

								const auto extension = rfind(begin, split, u8'.');
								if (extension != split)
								{
									const auto radical = utility::string_units_utf8(begin, extension);
									const auto radical_asset = engine::Asset(radical);

									const auto radical_it = find(files, radical_asset);
									if (radical_it != files.end())
									{
										files.call(radical_it, ext::overload(
											[&](AmbiguousFile &)
										{
										},
											[&](DirectoryFile &) {},
											[&](KnownFile &) {},
											[&](LoadingFile &) {},
											[&](LoadedFile &) {},
											[&](UniqueFile &)
										{
											files.erase(radical_it);

											if (!debug_verify(files.emplace<AmbiguousFile>(radical_asset)))
												return; // error
										}));
									}
									else
									{
										if (!debug_verify(files.emplace<UniqueFile>(radical_asset, file_asset)))
											return; // error
									}
								}
							}
						}

						if (split == end)
							break;

						begin = split + 1;
					}
				}

				for (auto key : keys)
				{
					bool found = false;

					for (auto & ambiguous_file : files.get<AmbiguousFile>())
					{
						const auto file_it = ext::find(ambiguous_file.files, key);
						if (file_it != ambiguous_file.files.end())
						{
							ambiguous_file.files.erase(file_it);

							if (ambiguous_file.files.size() == 1)
							{
								const auto unique_file = ext::front(ambiguous_file.files);

								const auto radical_asset = files.get_key(ambiguous_file);
								files.erase(find(files, radical_asset));

								if (!debug_verify(files.emplace<UniqueFile>(radical_asset, unique_file)))
									return; // error
							}

							found = true;
							break;
						}
					}
					if (found)
						continue;

					for (auto & unique_file : files.get<UniqueFile>())
					{
						if (unique_file.file == key)
						{
							const auto radical_asset = files.get_key(unique_file);
							files.erase(find(files, radical_asset));

							found = true;
							break;
						}
					}
					if (found)
						continue;

					const auto file_it = find(files, key);
					if (debug_verify(file_it != files.end()))
					{
						files.call(file_it, ext::overload(
							[&](AmbiguousFile &)
						{
							debug_unreachable(key, " is ambigous?");
						},
							[&](DirectoryFile &)
						{
							// ignore directories
						},
							[&](KnownFile & debug_expression(y))
						{
							if (debug_assert(y.directory == x.directory))
							{
								files.erase(file_it);
							}
						},
							[&](LoadingFile & y)
						{
							debug_assert(y.directory == x.directory);
						},
							[&](LoadedFile & y)
						{
							debug_assert(y.directory == x.directory);
						},
							[&](UniqueFile &)
						{
							debug_unreachable(key, " is unique?");
						}));
					}
				}
			}

			void operator () (MessageLoadGlobal && x)
			{
				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				if (!load_file(global, x.filetype, underlying_file.first, x.file, underlying_file.second, x.readycall, x.unreadycall, std::move(x.data)))
					return; // error
			}

			void operator () (MessageLoadLocal && x)
			{
				const auto underlying_owner = find_underlying_file(x.owner);
				if (!debug_verify(underlying_owner.second != files.end()))
					return; // error

				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				const auto success = files.call(underlying_owner.second, ext::overload(
					[&](AmbiguousFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					const auto count = y.remaining_count & INT_MAX;
					return debug_verify(y.attachments.insert(y.attachments.end() - count, underlying_file.first));
				},
					[&](LoadedFile & y)
				{
					return debug_verify(y.attachments.try_emplace_back(underlying_file.first));
				},
					[&](UniqueFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return; // error

				if (!load_file(underlying_owner.first, x.filetype, underlying_file.first, x.file, underlying_file.second, x.readycall, x.unreadycall, std::move(x.data)))
					return; // error
				// todo undo on failure
			}

			void operator () (MessageLoadDependency && x)
			{
				const auto underlying_owner = find_underlying_file(x.owner);
				if (!debug_verify(underlying_owner.second != files.end()))
					return; // error

				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				const auto success = files.call(underlying_owner.second, ext::overload(
					[&](AmbiguousFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					if (files.contains<LoadedFile>(underlying_file.second))
					{
						const auto count = y.remaining_count & INT_MAX;
						return debug_verify(y.attachments.insert(y.attachments.end() - count, underlying_file.first));
					}
					else
					{
						if (debug_verify(y.attachments.push_back(underlying_file.first)))
						{
							y.remaining_count++;
							return true;
						}
						return false;
					}
				},
					[&](LoadedFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is already loaded and cannot be an owner");
				},
					[&](UniqueFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return; // error

				if (!load_file(underlying_owner.first, x.filetype, underlying_file.first, x.file, underlying_file.second, x.readycall, x.unreadycall, std::move(x.data)))
					return; // error
				// todo undo on failure
			}

			void operator () (MessageLoadInit && x)
			{
				const auto file_it = find(files, x.file);
				if (!debug_assert(file_it != files.end()))
					return;

				files.call(file_it, ext::overload(
					[&](AmbiguousFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](DirectoryFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](KnownFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](LoadingFile &)
				{
					debug_fail("init should not be called on loading files");
				},
					[&](LoadedFile & y)
				{
					make_loading(x.file, file_it, std::move(y));
				},
					[&](UniqueFile &)
				{
					debug_fail("invalid file ", x.file);
				}));
			}

			void operator () (MessageLoadDone && x)
			{
				const auto file_it = find(files, x.file);
				if (!debug_assert(file_it != files.end()))
					return;

				files.call(file_it, ext::overload(
					[&](AmbiguousFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](DirectoryFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](KnownFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](LoadingFile & y)
				{
					y.remaining_count &= INT32_MAX;
					if (y.remaining_count == 0)
					{
						finish_loading(x.file, file_it, std::move(y));
					}
				},
					[&](LoadedFile &)
				{
					debug_fail("invalid file ", x.file);
				},
					[&](UniqueFile &)
				{
					debug_fail("invalid file ", x.file);
				}));
			}

			void operator () (MessageRegisterFiletype && x)
			{
				const auto filetype_ptr = filetypes.emplace<Filetype>(x.filetype);
				if (!debug_verify(filetype_ptr))
					return; // error

				filetype_ptr->callback = ext::heap_shared_ptr<Filetype::Callback>(utility::in_place, x.loadcall, x.unloadcall, std::move(x.data));
			}

			void operator () (MessageRegisterLibrary && x)
			{
				scanning_directory = x.directory;

				const auto file_it = find(files, x.directory);
				if (file_it == files.end())
				{
					if (!debug_verify(files.emplace<DirectoryFile>(x.directory)))
						return; // error
				}
				else
				{
					if (!debug_verify(files.contains<DirectoryFile>(file_it)))
						return; // error
				}
			}

			void operator () (MessageUnloadGlobal && x)
			{
				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				remove_file(underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnloadLocal && x)
			{
				const auto underlying_owner = find_underlying_file(x.owner);
				if (!debug_verify(underlying_owner.second != files.end()))
					return; // error

				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				const auto success = files.call(underlying_owner.second, ext::overload(
					[&](AmbiguousFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					debug_assert(y.remaining_count < y.attachments.end() - file_it);
					y.attachments.erase(file_it);

					return true;
				},
					[&](LoadedFile & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					y.attachments.erase(file_it);

					return true;
				},
					[&](UniqueFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return;

				remove_file(underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnloadDependency && x)
			{
				const auto underlying_owner = find_underlying_file(x.owner);
				if (!debug_verify(underlying_owner.second != files.end()))
					return; // error

				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				const auto success = files.call(underlying_owner.second, ext::overload(
					[&](AmbiguousFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					debug_assert(y.attachments.end() - file_it <= y.remaining_count);
					y.attachments.erase(file_it);

					y.remaining_count--;
					if (y.remaining_count == 0)
					{
						finish_loading(underlying_owner.first, underlying_owner.second, std::move(y));
					}

					return true;
				},
					[&](LoadedFile & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					y.attachments.erase(file_it);

					return true;
				},
					[&](UniqueFile &) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return;

				remove_file(underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnregisterFiletype && x)
			{
				const auto filetype_it = find(filetypes, x.filetype);
				if (!debug_verify(filetype_it != filetypes.end()))
					return; // error

				filetypes.erase(filetype_it);
			}

			void operator () (MessageUnregisterLibrary &&)
			{
			}
		};
		visit(ProcessMessage{}, std::move(message));
	}

	void loader_update(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Message>()))
			return;

		Message && message = utility::any_cast<Message &&>(std::move(data));

		if (scanning_directory != engine::Asset{})
		{
			if (utility::holds_alternative<MessageFileScan>(message) && utility::get<MessageFileScan>(message).directory == scanning_directory)
			{
				process_message(std::move(message));

				auto begin = delayed_messages.begin();
				const auto end = delayed_messages.end();
				for (; begin != end;)
				{
					const bool starting_new_scan = utility::holds_alternative<MessageRegisterLibrary>(*begin);

					process_message(std::move(*begin));
					++begin;

					if (starting_new_scan)
					{
						for (auto it = begin; it != end; ++it)
						{
							if (utility::holds_alternative<MessageFileScan>(*it) && utility::get<MessageFileScan>(*it).directory == scanning_directory)
							{
								process_message(std::move(*it));

								delayed_messages.erase(utility::stable, it);
								goto next; // todo
							}
						}
						break;
					}
				next:
					;
				}
				delayed_messages.erase(utility::stable, delayed_messages.begin(), begin);
			}
			else
			{
				debug_verify(delayed_messages.push_back(std::move(message)));
			}
		}
		else
		{
			process_message(std::move(message));
		}
	}
}

namespace
{
	void file_scan(engine::Asset directory, utility::heap_string_utf8 && files_, utility::any & /*data*/)
	{
		loader_update(*::module_taskscheduler, strand, Message(utility::in_place_type<MessageFileScan>, directory, std::move(files_)));
	}

	void ReadData::file_load(core::ReadStream && stream, utility::any & data)
	{
		ReadData * const read_data = utility::any_cast<ReadData>(&data);
		if (!debug_assert(read_data))
			return;

		ext::heap_shared_ptr<Filetype::Callback> filetypecall_ptr = read_data->filetype_callback.lock();
		if (!debug_verify(filetypecall_ptr))
			return;

		ext::heap_shared_ptr<FileCallData> filecall_ptr = read_data->file_callback.lock();
		if (!debug_verify(filecall_ptr))
			return;

		if (filecall_ptr->ready)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageLoadInit>, read_data->file));

			for (auto && call : filecall_ptr->calls)
			{
				call.second.unreadycall(call.second.data, call.first);
			}
			filecall_ptr->ready = false;
		}

		filetypecall_ptr->loadcall(std::move(stream), filetypecall_ptr->data, read_data->file);

		engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageLoadDone>, read_data->file));
	}
}

namespace engine
{
	namespace file
	{
		loader::~loader()
		{
			core::sync::Event<true> barrier;

			engine::task::post_work(
				*::module_taskscheduler,
				strand,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset /*strand*/, utility::any && data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<core::sync::Event<true> *>()))
					return;

				filetypes.clear(); // todo should already be empty
				files.clear(); // todo should already be empty

				core::sync::Event<true> * barrier = utility::any_cast<core::sync::Event<true> *>(data);
				barrier->set();
			},
				&barrier);

			barrier.wait();

			::module_filesystem = nullptr;
			::module_taskscheduler = nullptr;
		}

		loader::loader(engine::task::scheduler & taskscheduler, system & filesystem)
		{
			if (!debug_assert(::module_taskscheduler == nullptr))
				return;

			if (!debug_assert(::module_filesystem == nullptr))
				return;

			::module_taskscheduler = &taskscheduler;
			::module_filesystem = &filesystem;
		}

		void register_library(loader & /*loader*/, engine::Asset directory)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageRegisterLibrary>, directory));

#if MODE_DEBUG
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES;
#endif
			engine::file::scan(*::module_filesystem, directory, strand, file_scan, utility::any(), mode);
		}

		void unregister_library(loader & /*loader*/, engine::Asset directory)
		{
#if MODE_DEBUG
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH;
			engine::file::remove_watch(*::module_filesystem, directory, mode);
#endif

			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageUnregisterLibrary>, directory));
		}

		void register_filetype(loader & /*loader*/, engine::Asset filetype, load_callback * loadcall, unload_callback * unloadcall, utility::any && data)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageRegisterFiletype>, filetype, loadcall, unloadcall, std::move(data)));
		}

		void unregister_filetype(loader & /*loader*/, engine::Asset filetype)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageUnregisterFiletype>, filetype));
		}

		void load_global(
			loader & /*loader*/,
			engine::Asset filetype,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageLoadGlobal>, filetype, file, readycall, unreadycall, std::move(data)));
		}

		void load_local(
			loader & /*loader*/,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageLoadLocal>, filetype, owner, file, readycall, unreadycall, std::move(data)));
		}

		void load_dependency(
			loader & /*loader*/,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageLoadDependency>, filetype, owner, file, readycall, unreadycall, std::move(data)));
		}

		void unload_global(
			loader & /*loader*/,
			engine::Asset file)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageUnloadGlobal>, file));
		}

		void unload_local(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageUnloadLocal>, owner, file));
		}

		void unload_dependency(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file)
		{
			engine::task::post_work(*::module_taskscheduler, strand, loader_update, Message(utility::in_place_type<MessageUnloadDependency>, owner, file));
		}
	}
}
