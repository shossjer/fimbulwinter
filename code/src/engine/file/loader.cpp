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
#include "utility/overload.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/variant.hpp"
#include "utility/weak_ptr.hpp"

#include <atomic>
#include <memory>

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
		engine::file::loader_impl * impl; // todo can this be removed?
		engine::Asset file;
		ext::heap_weak_ptr<Filetype::Callback> filetype_callback;
		ext::heap_weak_ptr<FileCallData> file_callback;

		static void file_load(engine::file::system & filesystem, core::ReadStream && stream, utility::any & data);
	};

	constexpr auto global = engine::Asset{};
	constexpr auto strand = engine::Asset("_file_loader_");
}

namespace
{
	struct MessageFileScan
	{
		engine::Asset directory;
		utility::heap_string_utf8 existing_files;
		utility::heap_string_utf8 removed_files;
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

	struct Task
	{
		engine::file::loader_impl * impl;
		Message message;

		template <typename ...Ps>
		explicit Task(engine::file::loader_impl & impl, Ps && ...ps)
			: impl(&impl)
			, message(std::forward<Ps>(ps)...)
		{}
	};
}

namespace engine
{
	namespace file
	{
		struct loader_impl
		{
			engine::task::scheduler * taskscheduler;
			engine::file::system * filesystem;

			engine::Asset scanning_directory{};
			utility::heap_vector<Message> delayed_messages;
		};
	}
}

namespace
{
	utility::spinlock singelton_lock;
	utility::optional<engine::file::loader_impl> singelton;

	engine::file::loader_impl * create_impl()
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		if (singelton)
			return nullptr;

		singelton.emplace();

		return &singelton.value();
	}

	void destroy_impl(engine::file::loader_impl & /*impl*/)
	{
		std::lock_guard<utility::spinlock> guard(singelton_lock);

		singelton.reset();
	}
}

namespace
{
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
		utility::heap_vector<engine::Asset, utility::heap_string_utf8> metas;

		utility::heap_vector<engine::Asset, engine::Asset> radicals;

		explicit AmbiguousFile(utility::heap_vector<engine::Asset, engine::Asset> && radicals)
			: radicals(std::move(radicals))
		{}

		explicit AmbiguousFile(utility::heap_vector<engine::Asset, utility::heap_string_utf8> && metas, utility::heap_vector<engine::Asset, engine::Asset> && radicals)
			: metas(std::move(metas))
			, radicals(std::move(radicals))
		{}
	};

	struct DirectoryFile
	{
	};

	struct RadicalFile
	{
		engine::Asset directory;
		engine::Asset file;
	};

	struct UniqueFile
	{
		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		utility::heap_vector<engine::Asset, engine::Asset> radicals;
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<AmbiguousFile>,
		utility::heap_storage<DirectoryFile>,
		utility::heap_storage<RadicalFile>,
		utility::heap_storage<UniqueFile>
	>
	files;

	std::pair<engine::Asset, decltype(files.end())> find_underlying_file(engine::Asset file)
	{
		const auto file_it = find(files, file);
		if (file_it != files.end())
		{
			if (const RadicalFile * const radical_file = files.get<RadicalFile>(file_it))
				return std::make_pair(radical_file->file, find(files, radical_file->file));
		}
		return std::make_pair(file, file_it);
	}

	struct RadicalLoad
	{
		engine::Asset file;
	};

	struct LoadingLoad
	{
		engine::Asset filetype;
		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		FileCallPtr call_ptr;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		std::int32_t previous_count; // the number of attachments after loading completed
		std::int32_t remaining_count; // the remaining number of attachments that are required (the most significant bit is set if the file itself is not yet finished reading)

		explicit LoadingLoad(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, call_ptr(std::move(call_ptr))
			, previous_count(-1)
			, remaining_count(INT_MIN)
		{}

		explicit LoadingLoad(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
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

	struct LoadedLoad
	{
		engine::Asset filetype;
		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		FileCallPtr call_ptr;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		explicit LoadedLoad(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, FileCallPtr && call_ptr, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, call_ptr(std::move(call_ptr))
			, owners(std::move(owners))
			, attachments(std::move(attachments))
		{}
	};

	core::container::Collection
	<
		engine::Asset,
		utility::heap_storage_traits,
		utility::heap_storage<RadicalLoad>,
		utility::heap_storage<LoadingLoad>,
		utility::heap_storage<LoadedLoad>
	>
	loads;

	std::pair<engine::Asset, decltype(loads.end())> find_underlying_load(engine::Asset file)
	{
		const auto load_it = find(loads, file);
		if (load_it != loads.end())
		{
			if (const RadicalLoad * const radical_load = loads.get<RadicalLoad>(load_it))
				return std::make_pair(radical_load->file, find(loads, radical_load->file));
		}
		return std::make_pair(file, load_it);
	}

	bool make_loaded(engine::file::loader_impl & impl, engine::Asset file, decltype(loads.end()) load_it, LoadingLoad tmp)
	{
		// todo replace
		loads.erase(load_it);
		// todo on failure the file is lost
		const auto loaded_file = loads.emplace<LoadedLoad>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.call_ptr), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loaded_file))
			return false;

		engine::task::post_work(
			*impl.taskscheduler,
			file,
			[](engine::task::scheduler & /*scheduler*/, engine::Asset strand_, utility::any && data)
		{
			// note strand is the underlying file
			if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
			{
				FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
				FileCallData & call_data = *call_ptr;

				if (debug_assert(!call_data.ready))
				{
					for (auto && call : call_data.calls)
					{
						call.second.readycall(call.second.data, call.first, strand_);
					}
					call_data.ready = true;
				}
			}
		},
			loaded_file->call_ptr);

		return true;
	}

	bool make_loading(engine::Asset file, decltype(loads.end()) load_it, LoadedLoad tmp)
	{
		// todo replace
		loads.erase(load_it);
		// todo on failure the file is lost
		const auto loading_file = loads.emplace<LoadingLoad>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.call_ptr), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loading_file))
			return false;

		return true;
	}

	void remove_attachments(engine::file::loader_impl & impl, utility::heap_vector<engine::Asset, engine::Asset> && relations)
	{
		while (!ext::empty(relations))
		{
			const utility::heap_vector<engine::Asset, engine::Asset>::value_type relation = ext::back(std::move(relations));
			ext::pop_back(relations);

			const auto load_it = find(loads, relation.second);
			if (!debug_assert(load_it != loads.end(), relation.second, " cannot be found"))
				break;

			loads.call(load_it, ext::overload(
				[&](RadicalLoad &) { debug_unreachable(); },
				[&](LoadingLoad & x)
			{
				const auto owner_it = ext::find(x.owners, relation.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					const auto mode = engine::file::flags::ADD_WATCH;
					engine::file::remove_watch(*impl.filesystem, x.directory, std::move(x.filepath), mode);
#endif

					engine::task::post_work(
						*impl.taskscheduler,
						relation.second,
						[](engine::task::scheduler & /*scheduler*/, engine::Asset strand_, utility::any && data)
					{
						// note strand is the underlying file
						if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
						{
							FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
							FileCallData & call_data = *call_ptr;

							if (debug_assert(call_data.ready))
							{
								for (auto && call : call_data.calls)
								{
									call.second.unreadycall(call.second.data, call.first, strand_);
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

					loads.erase(load_it);
				}
			},
				[&](LoadedLoad & x)
			{
				const auto owner_it = ext::find(x.owners, relation.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					const auto mode = engine::file::flags::ADD_WATCH;
					engine::file::remove_watch(*impl.filesystem, x.directory, std::move(x.filepath), mode);
#endif

					engine::task::post_work(
						*impl.taskscheduler,
						relation.second,
						[](engine::task::scheduler & /*scheduler*/, engine::Asset strand_, utility::any && data)
					{
						// note strand is the underlying file
						if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
						{
							FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
							FileCallData & call_data = *call_ptr;

							if (debug_assert(call_data.ready))
							{
								for (auto && call : call_data.calls)
								{
									call.second.unreadycall(call.second.data, call.first, strand_);
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
								*impl.taskscheduler,
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

					loads.erase(load_it);
				}
			}));
		}
	}

	bool load_new(
		engine::file::loader_impl & impl,
		engine::Asset owner,
		engine::Asset file,
		decltype(files.end()) file_it,
		engine::Asset actual_file,
		engine::Asset filetype,
		engine::file::ready_callback * readycall,
		engine::file::unready_callback * unreadycall,
		utility::any && data)
	{
		UniqueFile * const unique_file = files.get<UniqueFile>(file_it);
		if (!debug_assert(unique_file))
			return false;

		const auto filetype_it = find(filetypes, filetype);
		if (!debug_verify(filetype_it != filetypes.end()))
			return false; // error

		FileCallPtr call_ptr(utility::in_place);
		if (!debug_verify(call_ptr->calls.try_emplace_back(actual_file, FileCallData::Callback{readycall, unreadycall, std::move(data)})))
			return false; // error

		LoadingLoad * const loading_load = loads.emplace<LoadingLoad>(file, filetype, unique_file->directory, utility::heap_string_utf8(unique_file->filepath), std::move(call_ptr));
		if (!debug_verify(loading_load))
			return false; // error

		if (!debug_verify(loading_load->owners.try_emplace_back(owner)))
		{
			loads.erase(find(loads, file));
			return false; // error
		}

		Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
		if (!debug_assert(filetype_ptr))
		{
			loads.erase(find(loads, file));
			return false;
		}

#if MODE_DEBUG
		const auto mode = engine::file::flags::ADD_WATCH; // todo ought to add engine::file::flags::RECURSE_DIRECTORIES if filepath contains subdir, but since a recursive scan watch has already been made it might be fine anyway?
#else
		const auto mode = engine::file::flags{};
#endif
		engine::file::read(*impl.filesystem, loading_load->directory, utility::heap_string_utf8(loading_load->filepath), file, ReadData::file_load, ReadData{&impl, file, ext::heap_weak_ptr<Filetype::Callback>(filetype_ptr->callback), ext::heap_weak_ptr<FileCallData>(loading_load->call_ptr)}, mode);

		return true;
	}

	bool load_old(
		engine::file::loader_impl & impl,
		engine::Asset owner,
		engine::Asset file,
		decltype(loads.end()) file_it,
		engine::Asset actual_file,
		engine::Asset filetype,
		engine::file::ready_callback * readycall,
		engine::file::unready_callback * unreadycall,
		utility::any && data)
	{
		return loads.call(file_it, ext::overload(
			[&](RadicalLoad &) -> bool { debug_unreachable(); },
			[&](LoadingLoad & y)
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
			[&](LoadedLoad & y)
		{
			if (!debug_assert(y.filetype == filetype))
				return false;

			if (!debug_verify(y.owners.try_emplace_back(owner)))
				return false; // error

			engine::task::post_work(
				*impl.taskscheduler,
				file,
				[](engine::task::scheduler & /*scheduler*/, engine::Asset strand_, utility::any && data)
			{
				// note strand is the underlying file
				if (debug_assert(data.type_id() == utility::type_id<FileCallDataPlusOne>()))
				{
					FileCallDataPlusOne call_ptr = utility::any_cast<FileCallDataPlusOne &&>(std::move(data));
					FileCallData & call_data = *call_ptr.call_ptr;

					if (!debug_verify(call_data.calls.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(call_ptr.file_alias), std::forward_as_tuple(call_ptr.new_call.readycall, call_ptr.new_call.unreadycall, std::move(call_ptr.new_call.data)))))
						return;

					if (debug_assert(call_data.ready))
					{
						auto && call = ext::back(call_data.calls);

						call.second.readycall(call.second.data, call.first, strand_);
					}
				}
			},
				FileCallDataPlusOne{y.call_ptr, actual_file, FileCallData::Callback{readycall, unreadycall, std::move(data)}});

			return true;
		}));
	}

	bool remove_file(
		engine::file::loader_impl & impl,
		engine::Asset file,
		decltype(loads.end()) load_it)
	{
		return loads.call(load_it, ext::overload(
			[&](RadicalLoad &) -> bool { debug_unreachable(); },
			[&](LoadingLoad & y)
		{
			const auto owner_it = ext::find(y.owners, global);
			if (!debug_verify(owner_it != y.owners.end(), "file ", file, " is not global"))
				return false; // error

			y.owners.erase(owner_it);

			if (ext::empty(y.owners))
			{
#if MODE_DEBUG
				const auto mode = engine::file::flags::ADD_WATCH;
				engine::file::remove_watch(*impl.filesystem, y.directory, std::move(y.filepath), mode);
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
								*impl.taskscheduler,
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
				remove_attachments(impl, std::move(relations));
			}
			return true;
		},
			[&](LoadedLoad & y)
		{
			const auto owner_it = ext::find(y.owners, global);
			if (!debug_verify(owner_it != y.owners.end(), "file ", file, " is not global"))
				return false; // error

			y.owners.erase(owner_it);

			if (ext::empty(y.owners))
			{
#if MODE_DEBUG
				const auto mode = engine::file::flags::ADD_WATCH;
				engine::file::remove_watch(*impl.filesystem, y.directory, std::move(y.filepath), mode);
#endif

				engine::task::post_work(
					*impl.taskscheduler,
					file,
					[](engine::task::scheduler & /*scheduler*/, engine::Asset strand_, utility::any && data)
				{
					// note strand is the underlying file
					if (debug_assert(data.type_id() == utility::type_id<FileCallPtr>()))
					{
						FileCallPtr call_ptr = utility::any_cast<FileCallPtr &&>(std::move(data));
						FileCallData & call_data = *call_ptr;

						if (debug_assert(call_data.ready))
						{
							for (auto && call : call_data.calls)
							{
								call.second.unreadycall(call.second.data, call.first, strand_);
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
							*impl.taskscheduler,
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
				remove_attachments(impl, std::move(relations));
			}
			return true;
		}));
	}

	void finish_loading(engine::file::loader_impl & impl, engine::Asset file, decltype(loads.end()) load_it, LoadingLoad && loading_load)
	{
		utility::heap_vector<engine::Asset, engine::Asset> relations;
		if (debug_verify(relations.try_reserve(loading_load.owners.size())))
		{
			for (auto owner : loading_load.owners)
			{
				relations.try_emplace_back(utility::no_failure, owner, file);
			}
		}

		if (0 < loading_load.previous_count)
		{
			utility::heap_vector<engine::Asset, engine::Asset> relations_;
			if (debug_verify(relations_.try_reserve(loading_load.previous_count)))
			{
				const auto split_it = loading_load.attachments.begin() + loading_load.previous_count;
				for (auto attachment_it = loading_load.attachments.begin(); attachment_it != split_it; ++attachment_it)
				{
					relations_.try_emplace_back(utility::no_failure, file, *attachment_it);
				}
				loading_load.attachments.erase(loading_load.attachments.begin(), split_it);
			}
			remove_attachments(impl, std::move(relations_));
		}

		if (!make_loaded(impl, file, load_it, std::move(loading_load)))
			return; // error

		while (!ext::empty(relations))
		{
			const utility::heap_vector<engine::Asset, engine::Asset>::value_type relation = ext::back(std::move(relations));
			ext::pop_back(relations);

			if (relation.first == global)
				continue;

			const auto owner_it = find(loads, relation.first);
			if (LoadingLoad * const loading_owner = loads.get<LoadingLoad>(owner_it))
			{
				const auto count = loading_owner->remaining_count & INT_MAX;
				const auto split_it = loading_owner->attachments.end() - count;

				const auto attachment_it = ext::find(split_it, loading_owner->attachments.end(), relation.second);
				if (attachment_it != loading_owner->attachments.end())
				{
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
							remove_attachments(impl, std::move(relations_));
						}

						if (!make_loaded(impl, relation.first, owner_it, std::move(*loading_owner)))
							return; // error
					}
				}
			}
		}
	}

	bool add_attachment(decltype(loads.end()) owner_it, engine::Asset attachment)
	{
		return loads.call(owner_it, ext::overload(
			[&](RadicalLoad &) -> bool
		{
			debug_unreachable("radicals cannot be owners");
		},
			[&](LoadingLoad & y)
		{
			const auto count = y.remaining_count & INT_MAX;
			return debug_verify(y.attachments.insert(y.attachments.end() - count, attachment));
		},
			[&](LoadedLoad & y)
		{
			return debug_verify(y.attachments.try_emplace_back(attachment));
		}));
	}

	bool add_dependency_loaded(decltype(loads.end()) owner_it, engine::Asset attachment)
	{
		return loads.call(owner_it, ext::overload(
			[&](RadicalLoad &) -> bool
		{
			debug_unreachable("radicals cannot be owners");
		},
			[&](LoadingLoad & y)
		{
			const auto count = y.remaining_count & INT_MAX;
			return debug_verify(y.attachments.insert(y.attachments.end() - count, attachment));
		},
			[&](LoadedLoad &)
		{
			return debug_fail("loaded files cannot be owners to dependencies");
		}));
	}

	bool add_dependency_unloaded(decltype(loads.end()) owner_it, engine::Asset attachment)
	{
		return loads.call(owner_it, ext::overload(
			[&](RadicalLoad &) -> bool
		{
			debug_unreachable("radicals cannot be owners");
		},
			[&](LoadingLoad & y)
		{
			if (debug_verify(y.attachments.push_back(attachment)))
			{
				y.remaining_count++;
				return true;
			}
			return false;
		},
			[&](LoadedLoad &)
		{
			return debug_fail("loaded files cannot be owners to dependencies");
		}));
	}

	void process_message(engine::file::loader_impl & impl, Message && message)
	{
		struct ProcessMessage
		{
			engine::file::loader_impl & impl;

			void operator () (MessageFileScan && x)
			{
				impl.scanning_directory = engine::Asset{};

				{
					auto begin = x.removed_files.begin();
					const auto end = x.removed_files.end();

					if (begin != end)
					{
						while (true)
						{
							const auto split = find(begin, end, u8';');
							if (!debug_assert(split != begin, "unexpected file without name"))
								return; // error

							const auto filepath = utility::string_units_utf8(begin, split);
							const auto file_asset = engine::Asset(filepath);

							const auto file_it = find(files, file_asset);
							if (debug_assert(file_it != files.end()))
							{
								files.call(file_it, ext::overload(
									[&](AmbiguousFile & y)
								{
									const auto meta_it = ext::find_if(y.metas, fun::first == x.directory && fun::second == filepath);
									if (debug_assert(meta_it != y.metas.end()))
									{
										y.metas.erase(meta_it);
										if (y.metas.size() == 1)
										{
											auto tmp_directory = ext::front(std::move(y.metas)).first;
											auto tmp_filepath = ext::front(std::move(y.metas)).second;
											auto tmp_radicals = std::move(y.radicals);
											files.erase(file_it);
											if (!debug_verify(files.emplace<UniqueFile>(file_asset, tmp_directory, std::move(tmp_filepath), std::move(tmp_radicals))))
												return; // error
										}
									}
								},
									[&](DirectoryFile &)
								{
									debug_unreachable(file_asset, " collides with a directory");
								},
									[&](RadicalFile &)
								{
									debug_unreachable(file_asset, " collides with a radical");
								},
									[&](UniqueFile & y)
								{
									if (y.radicals.size() == 0)
									{
										files.erase(file_it);
									}
									else if (y.radicals.size() == 1)
									{
										auto directory = ext::front(std::move(y.radicals)).first;
										auto file = ext::front(std::move(y.radicals)).second;
										files.erase(file_it);
										if (!debug_verify(files.emplace<RadicalFile>(file_asset, directory, file)))
											return; // error
									}
									else
									{
										auto radicals = std::move(y.radicals);
										files.erase(file_it);
										if (!debug_verify(files.emplace<AmbiguousFile>(file_asset, std::move(radicals))))
											return; // error
									}
								}));
							}

							const auto extension = rfind(begin, split, u8'.');
							if (extension != split)
							{
								const auto radical = utility::string_units_utf8(begin, extension);
								const auto radical_asset = engine::Asset(radical);

								const auto radical_it = find(files, radical_asset);
								if (debug_assert(radical_it != files.end()))
								{
									files.call(radical_it, ext::overload(
										[&](AmbiguousFile & y)
									{
										y.radicals.erase(ext::find_if(y.radicals, fun::first == x.directory && fun::second == file_asset));
										if (y.radicals.size() == 1 && ext::empty(y.metas))
										{
											auto tmp_directory = ext::front(std::move(y.radicals)).first;
											auto tmp_file = ext::front(std::move(y.radicals)).second;
											files.erase(radical_it);
											if (!debug_verify(files.emplace<RadicalFile>(file_asset, tmp_directory, tmp_file)))
												return; // error
										}
									},
										[&](DirectoryFile &)
									{
										debug_unreachable(radical_asset, " collides with a directory");
									},
										[&](RadicalFile &)
									{
										files.erase(radical_it);
									},
										[&](UniqueFile & y)
									{
										y.radicals.erase(ext::find_if(y.radicals, fun::first == x.directory && fun::second == file_asset));
									}));
								}
							}
						}
					}
				}

				{
					auto begin = x.existing_files.begin();
					const auto end = x.existing_files.end();

					if (begin != end)
					{
						while (true)
						{
							const auto split = find(begin, end, u8';');
							if (!debug_assert(split != begin, "unexpected file without name"))
								return; // error

							const auto filepath = utility::string_units_utf8(begin, split);
							const auto file_asset = engine::Asset(filepath);

							const auto file_it = find(files, file_asset);
							if (file_it != files.end())
							{
								files.call(file_it, ext::overload(
									[&](AmbiguousFile & y)
								{
									const auto meta_it = ext::find_if(y.metas, fun::first == x.directory && fun::second == filepath);
									if (meta_it == y.metas.end())
									{
										if (ext::empty(y.metas))
										{
											auto radicals = std::move(y.radicals);
											files.erase(file_it);
											if (!debug_verify(files.emplace<UniqueFile>(file_asset, x.directory, utility::heap_string_utf8(filepath), std::move(radicals))))
												return; // error
										}
										else
										{
											if (!debug_verify(y.metas.try_emplace_back(x.directory, utility::heap_string_utf8(filepath))))
												return; // error
										}
									}
								},
									[&](DirectoryFile &)
								{
									debug_unreachable(file_asset, " collides with a directory");
								},
									[&](RadicalFile & y)
								{
									utility::heap_vector<engine::Asset, engine::Asset> radicals;
									if (!debug_verify(radicals.try_emplace_back(y.directory, y.file)))
										return; // error

									files.erase(file_it);
									if (!debug_verify(files.emplace<UniqueFile>(file_asset, x.directory, utility::heap_string_utf8(filepath), std::move(radicals))))
										return; // error
								},
									[&](UniqueFile & y)
								{
									if (!(y.directory == x.directory && y.filepath == filepath))
									{
										utility::heap_vector<engine::Asset, utility::heap_string_utf8> metas;
										if (!debug_verify(metas.try_emplace_back(y.directory, std::move(y.filepath))))
											return; // error
										if (!debug_verify(metas.try_emplace_back(x.directory, utility::heap_string_utf8(filepath))))
											return; // error

										auto radicals = std::move(y.radicals);
										files.erase(file_it);
										if (!debug_verify(files.emplace<AmbiguousFile>(file_asset, std::move(metas), std::move(radicals))))
											return; // error
									}
								}));
							}
							else
							{
								if (!debug_verify(files.emplace<UniqueFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
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
											[&](AmbiguousFile & y)
										{
											if (!debug_verify(y.radicals.try_emplace_back(x.directory, file_asset)))
												return; // error
										},
											[&](DirectoryFile &)
										{
											debug_unreachable(radical_asset, " collides with a directory");
										},
											[&](RadicalFile & y)
										{
											utility::heap_vector<engine::Asset, engine::Asset> radicals;
											if (!debug_verify(radicals.try_emplace_back(y.directory, y.file)))
												return; // error
											if (!debug_verify(radicals.try_emplace_back(x.directory, file_asset)))
												return; // error

											files.erase(radical_it);
											if (!debug_verify(files.emplace<AmbiguousFile>(radical_asset, std::move(radicals))))
												return; // error
										},
											[&](UniqueFile & y)
										{
											if (!debug_verify(y.radicals.try_emplace_back(x.directory, file_asset)))
												return; // error
										}));
									}
									else
									{
										if (!debug_verify(files.emplace<RadicalFile>(radical_asset, x.directory, file_asset)))
											return; // error
									}
								}
							}

							if (split == end)
								break;

							begin = split + 1;
						}
					}
				}
			}

			void operator () (MessageLoadGlobal && x)
			{
				const auto underlying_load = find_underlying_load(x.file);
				if (underlying_load.second != loads.end())
				{
					if (!load_old(impl, global, underlying_load.first, underlying_load.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
						return; // error
				}
				else
				{
					const auto underlying_file = find_underlying_file(x.file);
					if (!debug_verify(underlying_file.second != files.end(), x.file, " cannot be found"))
						return; // error

					if (!files.call(underlying_file.second, ext::overload(
						[&](AmbiguousFile &) { return debug_fail(underlying_file.first, " is ambigous and cannot be loaded"); },
						[&](DirectoryFile &) { return debug_fail(underlying_file.first, " collides with a directory"); },
						[&](RadicalFile &) -> bool { debug_unreachable(underlying_file.first); },
						[&](UniqueFile &) { return true; })))
						return; // error

					const auto underlying_load_ = find_underlying_load(underlying_file.first);
					if (underlying_load_.second != loads.end())
					{
						if (!load_old(impl, global, underlying_load_.first, underlying_load_.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
					else
					{
						if (!load_new(impl, global, underlying_file.first, underlying_file.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
				}
			}

			void operator () (MessageLoadLocal && x)
			{
				const auto underlying_owner = find_underlying_load(x.owner);
				if (!debug_verify(underlying_owner.second != loads.end(), x.owner, " cannot be found"))
					return; // error

				const auto underlying_load = find_underlying_load(x.file);
				if (underlying_load.second != loads.end())
				{
					if (!add_attachment(underlying_owner.second, underlying_load.first))
						return; // error

					if (!load_old(impl, underlying_owner.first, underlying_load.first, underlying_load.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
						return; // error
				}
				else
				{
					const auto underlying_file = find_underlying_file(x.file);
					if (!debug_verify(underlying_file.second != files.end(), x.file, " cannot be found"))
						return; // error

					if (!files.call(underlying_file.second, ext::overload(
						[&](AmbiguousFile &) { return debug_fail(underlying_file.first, " is ambigous and cannot be loaded"); },
						[&](DirectoryFile &) { return debug_fail(underlying_file.first, " collides with a directory"); },
						[&](RadicalFile &) -> bool { debug_unreachable(underlying_file.first); },
						[&](UniqueFile &) { return true; })))
						return; // error

					if (!add_attachment(underlying_owner.second, underlying_file.first))
						return; // error

					const auto underlying_load_ = find_underlying_load(underlying_file.first);
					if (underlying_load_.second != loads.end())
					{
						if (!load_old(impl, underlying_owner.first, underlying_load_.first, underlying_load_.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
					else
					{
						if (!load_new(impl, underlying_owner.first, underlying_file.first, underlying_file.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
				}
			}

			void operator () (MessageLoadDependency && x)
			{
				const auto underlying_owner = find_underlying_load(x.owner);
				if (!debug_verify(underlying_owner.second != loads.end(), x.owner, " cannot be found"))
					return; // error

				const auto underlying_load = find_underlying_load(x.file);
				if (underlying_load.second != loads.end())
				{
					if (loads.contains<LoadedLoad>(underlying_load.second))
					{
						if (!add_dependency_loaded(underlying_owner.second, underlying_load.first))
							return; // error
					}
					else
					{
						if (!add_dependency_unloaded(underlying_owner.second, underlying_load.first))
							return; // error
					}

					if (!load_old(impl, underlying_owner.first, underlying_load.first, underlying_load.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
						return; // error
				}
				else
				{
					const auto underlying_file = find_underlying_file(x.file);
					if (!debug_verify(underlying_file.second != files.end(), x.file, " cannot be found"))
						return; // error

					if (!files.call(underlying_file.second, ext::overload(
						[&](AmbiguousFile &) { return debug_fail(underlying_file.first, " is ambigous and cannot be loaded"); },
						[&](DirectoryFile &) { return debug_fail(underlying_file.first, " collides with a directory"); },
						[&](RadicalFile &) -> bool { debug_unreachable(underlying_file.first); },
						[&](UniqueFile &) { return true; })))
						return; // error

					if (!add_dependency_unloaded(underlying_owner.second, underlying_file.first))
						return; // error

					const auto underlying_load_ = find_underlying_load(underlying_file.first);
					if (underlying_load_.second != loads.end())
					{
						if (!load_old(impl, underlying_owner.first, underlying_load_.first, underlying_load_.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
					else
					{
						if (!load_new(impl, underlying_owner.first, underlying_file.first, underlying_file.second, x.file, x.filetype, x.readycall, x.unreadycall, std::move(x.data)))
							return; // error
					}
				}
			}

			void operator () (MessageLoadInit && x)
			{
				const auto file_it = find(loads, x.file);
				if (!debug_assert(file_it != loads.end(), x.file, " cannot be found"))
					return;

				LoadedLoad * const loaded_load = loads.get<LoadedLoad>(file_it);
				if (!debug_verify(loaded_load))
					return; // error

				make_loading(x.file, file_it, std::move(*loaded_load));
			}

			void operator () (MessageLoadDone && x)
			{
				const auto file_it = find(loads, x.file);
				if (!debug_verify(file_it != loads.end(), x.file, " cannot be found")) // todo debug_check
					return;

				LoadingLoad * const loading_load = loads.get<LoadingLoad>(file_it);
				if (!debug_verify(loading_load))
					return; // error

				loading_load->remaining_count &= INT32_MAX;
				if (loading_load->remaining_count == 0)
				{
					finish_loading(impl, x.file, file_it, std::move(*loading_load));
				}
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
				impl.scanning_directory = x.directory;

				const auto file_it = find(files, x.directory);
				if (file_it != files.end())
				{
					const auto success = files.call(file_it, ext::overload(
						[&](AmbiguousFile &)
					{
						return debug_fail("directory ", x.directory, " collides with ambiguous file");
					},
						[&](DirectoryFile &)
					{
						return debug_fail("directory ", x.directory, " already registered");
					},
						[&](RadicalFile &)
					{
						return debug_fail("directory ", x.directory, " collides with radical file");
					},
						[&](UniqueFile &)
					{
						return debug_fail("directory ", x.directory, " collides with unique file");
					}));
					if (!success)
						return; // error
				}
				else
				{
					if (!debug_verify(files.emplace<DirectoryFile>(x.directory)))
						return; // error
				}
			}

			void operator () (MessageUnloadGlobal && x)
			{
				const auto underlying_file = find_underlying_load(x.file);
				if (!debug_verify(underlying_file.second != loads.end(), x.file, " cannot be found"))
					return; // error

				remove_file(impl, underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnloadLocal && x)
			{
				const auto underlying_owner = find_underlying_load(x.owner);
				if (!debug_verify(underlying_owner.second != loads.end(), x.owner, " cannot be found"))
					return; // error

				const auto underlying_file = find_underlying_load(x.file);
				if (!debug_verify(underlying_file.second != loads.end(), x.file, " cannot be found"))
					return; // error

				const auto success = loads.call(underlying_owner.second, ext::overload(
					[&](RadicalLoad &) -> bool
				{
					debug_unreachable("find_underlying_load returned a radical ", x.owner, " -> ", underlying_owner.first);
				},
					[&](LoadingLoad & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					debug_assert(y.remaining_count < y.attachments.end() - file_it);
					y.attachments.erase(file_it);

					return true;
				},
					[&](LoadedLoad & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					y.attachments.erase(file_it);

					return true;
				}));
				if (!success)
					return;

				remove_file(impl, underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnloadDependency && x)
			{
				const auto underlying_owner = find_underlying_load(x.owner);
				if (!debug_verify(underlying_owner.second != loads.end(), x.owner, " cannot be found"))
					return; // error

				const auto underlying_file = find_underlying_load(x.file);
				if (!debug_verify(underlying_file.second != loads.end(), x.file, " cannot be found"))
					return; // error

				const auto success = loads.call(underlying_owner.second, ext::overload(
					[&](RadicalLoad &) -> bool
				{
					debug_unreachable("find_underlying_load returned a radical ", x.owner, " -> ", underlying_owner.first);
				},
					[&](LoadingLoad & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					debug_assert(y.attachments.end() - file_it <= y.remaining_count);
					y.attachments.erase(file_it);

					y.remaining_count--;
					if (y.remaining_count == 0)
					{
						finish_loading(impl, underlying_owner.first, underlying_owner.second, std::move(y));
					}

					return true;
				},
					[&](LoadedLoad & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					y.attachments.erase(file_it);

					return true;
				}));
				if (!success)
					return;

				remove_file(impl, underlying_file.first, underlying_file.second);
			}

			void operator () (MessageUnregisterFiletype && x)
			{
				const auto filetype_it = find(filetypes, x.filetype);
				if (!debug_verify(filetype_it != filetypes.end()))
					return; // error

				filetypes.erase(filetype_it);
			}

			void operator () (MessageUnregisterLibrary && x)
			{
				const auto file_it = find(files, x.directory);
				if (!debug_verify(file_it != files.end(), x.directory, " cannot be found"))
					return; // error

				if (!debug_verify(files.contains<DirectoryFile>(file_it)))
					return; // error

				files.erase(file_it);
			}
		};
		visit(ProcessMessage{impl}, std::move(message));
	}

	void loader_update(engine::task::scheduler & /*taskscheduler*/, engine::Asset /*strand*/, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<Task>()))
			return;

		Task && task = utility::any_cast<Task &&>(std::move(data));

		if (task.impl->scanning_directory != engine::Asset{})
		{
			if (utility::holds_alternative<MessageFileScan>(task.message) && utility::get<MessageFileScan>(task.message).directory == task.impl->scanning_directory)
			{
				process_message(*task.impl, std::move(task.message));

				auto begin = task.impl->delayed_messages.begin();
				const auto end = task.impl->delayed_messages.end();
				for (; begin != end;)
				{
					const bool starting_new_scan = utility::holds_alternative<MessageRegisterLibrary>(*begin);

					process_message(*task.impl, std::move(*begin));
					++begin;

					if (starting_new_scan)
					{
						for (auto it = begin; it != end; ++it)
						{
							if (utility::holds_alternative<MessageFileScan>(*it) && utility::get<MessageFileScan>(*it).directory == task.impl->scanning_directory)
							{
								process_message(*task.impl, std::move(*it));

								task.impl->delayed_messages.erase(utility::stable, it);
								goto next; // todo
							}
						}
						break;
					}
				next:
					;
				}
				task.impl->delayed_messages.erase(utility::stable, task.impl->delayed_messages.begin(), begin);
			}
			else
			{
				debug_verify(task.impl->delayed_messages.push_back(std::move(task.message)));
			}
		}
		else
		{
			process_message(*task.impl, std::move(task.message));
		}
	}
}

namespace
{
	void file_scan(engine::file::system & /*filesystem*/, engine::Asset directory, utility::heap_string_utf8 && existing_files, utility::heap_string_utf8 && removed_files, utility::any & data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<engine::file::loader *>()))
			return;

		engine::file::loader & loader = *utility::any_cast<engine::file::loader *>(std::move(data));

		loader_update(*loader->taskscheduler, strand, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageFileScan>, directory, std::move(existing_files), std::move(removed_files)));
	}

	void ReadData::file_load(engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
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
			engine::task::post_work(*read_data->impl->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *read_data->impl, utility::in_place_type<MessageLoadInit>, read_data->file));

			for (auto && call : filecall_ptr->calls)
			{
				call.second.unreadycall(call.second.data, call.first, read_data->file);
			}
			filecall_ptr->ready = false;
		}

		filetypecall_ptr->loadcall(std::move(stream), filetypecall_ptr->data, read_data->file);

		engine::task::post_work(*read_data->impl->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *read_data->impl, utility::in_place_type<MessageLoadDone>, read_data->file));
	}
}

namespace engine
{
	namespace file
	{
		void loader::destruct(loader_impl & impl)
		{
			core::sync::Event<true> barrier;

			engine::task::post_work(
				*impl.taskscheduler,
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

			destroy_impl(impl);
		}

		loader_impl * loader::construct(engine::task::scheduler & taskscheduler, engine::file::system & filesystem)
		{
			loader_impl * const impl = create_impl();
			if (debug_verify(impl))
			{
				impl->taskscheduler = &taskscheduler;
				impl->filesystem = &filesystem;
			}
			return impl;
		}

		void register_library(loader & loader, engine::Asset directory)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageRegisterLibrary>, directory));

#if MODE_DEBUG
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES;
#endif
			engine::file::scan(*loader->filesystem, directory, strand, file_scan, &loader, mode);
		}

		void unregister_library(loader & loader, engine::Asset directory)
		{
#if MODE_DEBUG
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH;
			engine::file::remove_watch(*loader->filesystem, directory, mode);
#endif

			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageUnregisterLibrary>, directory));
		}

		void register_filetype(loader & loader, engine::Asset filetype, load_callback * loadcall, unload_callback * unloadcall, utility::any && data)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageRegisterFiletype>, filetype, loadcall, unloadcall, std::move(data)));
		}

		void unregister_filetype(loader & loader, engine::Asset filetype)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageUnregisterFiletype>, filetype));
		}

		void load_global(
			loader & loader,
			engine::Asset filetype,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageLoadGlobal>, filetype, file, readycall, unreadycall, std::move(data)));
		}

		void load_local(
			loader & loader,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageLoadLocal>, filetype, owner, file, readycall, unreadycall, std::move(data)));
		}

		void load_dependency(
			loader & loader,
			engine::Asset filetype,
			engine::Asset owner,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageLoadDependency>, filetype, owner, file, readycall, unreadycall, std::move(data)));
		}

		void unload_global(
			loader & loader,
			engine::Asset file)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageUnloadGlobal>, file));
		}

		void unload_local(
			loader & loader,
			engine::Asset owner,
			engine::Asset file)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageUnloadLocal>, owner, file));
		}

		void unload_dependency(
			loader & loader,
			engine::Asset owner,
			engine::Asset file)
		{
			engine::task::post_work(*loader->taskscheduler, strand, loader_update, utility::any(utility::in_place_type<Task>, *loader, utility::in_place_type<MessageUnloadDependency>, owner, file));
		}
	}
}
