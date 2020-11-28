#include "engine/file/loader.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Collection.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/file/system.hpp"

#include "utility/any.hpp"
#include "utility/algorithm/find.hpp"
#include "utility/functional/utility.hpp"
#include "utility/variant.hpp"

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
	struct FiletypeCallback;
	struct FileCallback;

	struct ReadData
	{
		engine::Asset file;
		std::weak_ptr<FiletypeCallback> filetype_callback;

		static void file_load(core::ReadStream && stream, utility::any & data);
	};

	constexpr auto global = engine::Asset{};
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

	core::container::PageQueue<utility::heap_storage<Message>> queue;
}

namespace
{
	engine::file::system * module_filesystem = nullptr;

	struct FiletypeCallback
	{
		engine::file::load_callback * loadcall;
		engine::file::unload_callback * unloadcall;
		utility::any data;

		explicit FiletypeCallback(engine::file::load_callback * loadcall, engine::file::unload_callback * unloadcall, utility::any data)
			: loadcall(loadcall)
			, unloadcall(unloadcall)
			, data(std::move(data))
		{}
	};

	struct Filetype
	{
		std::shared_ptr<FiletypeCallback> callback;
	};

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

		utility::heap_vector<engine::Asset, engine::file::ready_callback *, engine::file::unready_callback *> calls;
		utility::any data;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		std::int32_t previous_count; // the number of attachments after loading completed
		std::int32_t remaining_count; // the remaining number of attachments that are required (the most significant bit is set if the file itself is not yet finished reading)

		explicit LoadingFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, utility::any && data)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, data(std::move(data))
			, previous_count(-1)
			, remaining_count(INT_MIN)
		{}

		explicit LoadingFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, utility::heap_vector<engine::Asset, engine::file::ready_callback *, engine::file::unready_callback *> && calls, utility::any && data, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, calls(std::move(calls))
			, data(std::move(data))
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

		utility::heap_vector<engine::Asset, engine::file::ready_callback *, engine::file::unready_callback *> calls;
		utility::any data;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		explicit LoadedFile(engine::Asset filetype, engine::Asset directory, utility::heap_string_utf8 && filepath, utility::heap_vector<engine::Asset, engine::file::ready_callback *, engine::file::unready_callback *> && calls, utility::any && data, utility::heap_vector<engine::Asset> && owners, utility::heap_vector<engine::Asset> && attachments)
			: filetype(filetype)
			, directory(directory)
			, filepath(std::move(filepath))
			, calls(std::move(calls))
			, data(std::move(data))
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
		const auto loaded_file = files.emplace<LoadedFile>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.calls), std::move(tmp.data), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loaded_file))
			return false;

		for (auto && call : loaded_file->calls)
		{
			std::get<1>(call)(loaded_file->data, std::get<0>(call));
		}
		return true;
	}

	bool make_loading(engine::Asset file, decltype(files.end()) file_it, LoadedFile tmp)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		const auto loading_file = files.emplace<LoadingFile>(file, tmp.filetype, tmp.directory, std::move(tmp.filepath), std::move(tmp.calls), std::move(tmp.data), std::move(tmp.owners), std::move(tmp.attachments));
		if (!debug_verify(loading_file))
			return false;

		for (auto && call : loading_file->calls)
		{
			std::get<2>(call)(loading_file->data, std::get<0>(call));
		}
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
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath));
#endif

					for (auto && call : x.calls)
					{
						std::get<2>(call)(x.data, std::get<0>(call));
					}
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
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath));
#endif

					for (auto && call : x.calls)
					{
						std::get<2>(call)(x.data, std::get<0>(call));
					}
					const auto filetype_it = find(filetypes, x.filetype);
					if (debug_assert(filetype_it != filetypes.end()))
					{
						Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
						if (debug_assert(filetype_ptr))
						{
							FiletypeCallback * const filetype_callback = filetype_ptr->callback.get();
							if (debug_assert(filetype_callback))
							{
								filetype_callback->unloadcall(filetype_callback->data, relation.second);
							}
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

			// todo replace
			files.erase(file_it);
			// todo on failure the file is lost
			LoadingFile * const loading_file = files.emplace<LoadingFile>(file, filetype, copy.directory, std::move(copy.filepath), std::move(data));
			if (!debug_verify(loading_file))
				return false; // error

			if (!debug_verify(loading_file->calls.try_emplace_back(actual_file, readycall, unreadycall)))
			{
				files.erase(find(files, file));
				return false; // error
			}

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
			const auto mode = engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags{};
#endif
			engine::file::read(*::module_filesystem, loading_file->directory, utility::heap_string_utf8(loading_file->filepath), ReadData::file_load, ReadData{file, std::weak_ptr<FiletypeCallback>(filetype_ptr->callback)/*, std::weak_ptr<FileCallback>(loading_file->callback)*/}, mode);

			return true;
		},
			[&](LoadingFile & y)
		{
			if (!debug_assert(y.filetype == filetype))
				return false;

			if (!debug_verify(y.calls.try_emplace_back(actual_file, readycall, unreadycall)))
				return false;

			if (!debug_verify(y.owners.try_emplace_back(owner)))
			{
				ext::pop_back(y.calls);
				return false;
			}

			return true;
		},
			[&](LoadedFile & y)
		{
			if (!debug_assert(y.filetype == filetype))
				return false;

			if (!debug_verify(y.calls.try_emplace_back(actual_file, readycall, unreadycall)))
				return false; // error

			if (!debug_verify(y.owners.try_emplace_back(owner)))
			{
				ext::pop_back(y.calls);
				return false; // error
			}

			readycall(y.data, actual_file);

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
				engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

				if (0 <= y.previous_count)
				{
					const auto filetype_it = find(filetypes, y.filetype);
					if (debug_assert(filetype_it != filetypes.end()))
					{
						Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
						if (debug_assert(filetype_ptr))
						{
							FiletypeCallback * const filetype_callback = filetype_ptr->callback.get();
							if (debug_assert(filetype_callback))
							{
								filetype_callback->unloadcall(filetype_callback->data, file);
							}
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
				engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

				for (auto && call : y.calls)
				{
					std::get<2>(call)(y.data, std::get<0>(call));
				}
				const auto filetype_it = find(filetypes, y.filetype);
				if (debug_assert(filetype_it != filetypes.end()))
				{
					Filetype * const filetype_ptr = filetypes.get<Filetype>(filetype_it);
					if (debug_assert(filetype_ptr))
					{
						FiletypeCallback * const filetype_callback = filetype_ptr->callback.get();
						if (debug_assert(filetype_callback))
						{
							filetype_callback->unloadcall(filetype_callback->data, file);
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

				auto begin = x.files.begin();
				const auto end = x.files.end();

				if (begin != end)
				{
					while (true)
					{
						const auto split = find(begin, end, u8';');
						if (!debug_assert(split != begin, "unexpected file without name"))
							return; // error

						const auto type = *begin;
						++begin;

						const auto filepath = utility::string_units_utf8(begin, split);
						const auto file_asset = engine::Asset(filepath);

						switch (type)
						{
						case '+':
						{
							const auto file_it = find(files, file_asset);
							if (file_it != files.end())
							{
								files.call(file_it, ext::overload(
									[&](AmbiguousFile &)
								{
									if (!debug_assert(find(begin, split, u8'.') == split))
										return;

									files.erase(file_it);

									debug_printline(file_asset, " is known: ", filepath);
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

									debug_printline(file_asset, " is known: ", filepath);
									if (!debug_verify(files.emplace<KnownFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
										return; // error
								}));
							}
							else
							{
								debug_printline(file_asset, " is known: ", filepath);
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

											debug_printline(file_asset, " is ambiguous: ", radical);
											if (!debug_verify(files.emplace<AmbiguousFile>(radical_asset)))
												return; // error
										}));
									}
									else
									{
										debug_printline(file_asset, " is unique: ", radical);
										if (!debug_verify(files.emplace<UniqueFile>(radical_asset, file_asset)))
											return; // error
									}
								}
							}
							break;
						}
						case '-':
							const auto file_it = find(files, file_asset);
							if (debug_verify(file_it != files.end(), ""))
							{
								const auto extension = rfind(begin, split, u8'.');
								if (extension != split)
								{
									const auto radical = utility::string_units_utf8(begin, extension);
									const auto radical_asset = engine::Asset(radical);

									const auto radical_it = find(files, radical_asset);
									if (debug_verify(radical_it != files.end()))
									{
										files.call(radical_it, ext::overload(
											[&](AmbiguousFile & y)
										{
											y.files.erase(ext::find(y.files, file_asset));

											if (y.files.size() == 1)
											{
												const auto unique_file = ext::front(y.files);

												files.erase(radical_it);

												debug_printline(file_asset, " is unique: ", radical);
												if (!debug_verify(files.emplace<UniqueFile>(radical_asset, unique_file)))
													return; // error
											}
										},
											[&](DirectoryFile &) {},
											[&](KnownFile &) {},
											[&](LoadingFile &) {},
											[&](LoadedFile &) {},
											[&](UniqueFile &)
										{
											files.erase(radical_it);
										}));
									}
								}

								files.call(file_it, ext::overload(
									[&](AmbiguousFile &)
								{
									debug_fail(file_asset, " is ambigous?");
								},
									[&](DirectoryFile &)
								{
									debug_unreachable(file_asset, " is a directory?");
								},
									[&](KnownFile & debug_expression(y))
								{
									if (debug_assert(y.directory == x.directory) && debug_assert(y.filepath == filepath))
									{
										files.erase(file_it);
									}
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
									debug_fail(file_asset, " is unique?");
								}));
							}
							break;
						}

						if (split == end)
							break;

						begin = split + 1;
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

				filetype_ptr->callback = std::make_shared<FiletypeCallback>(x.loadcall, x.unloadcall, std::move(x.data));
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

	void loader_update()
	{
		Message message;
		while (queue.try_pop(message))
		{
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

	core::async::Thread thread;
	std::atomic_int active(0);
	core::sync::Event<true> event;

	void file_loader()
	{
		event.wait();
		event.reset();

		while (active.load(std::memory_order_relaxed))
		{
			loader_update();

			event.wait();
			event.reset();
		}
	}
}

namespace
{
	void file_scan(engine::Asset directory, utility::heap_string_utf8 && files_, utility::any & /*data*/)
	{
		debug_verify(queue.try_emplace(utility::in_place_type<MessageFileScan>, directory, std::move(files_)));
		event.set();
	}

	void ReadData::file_load(core::ReadStream && stream, utility::any & data)
	{
		ReadData * const read_data = utility::any_cast<ReadData>(&data);
		if (!debug_assert(read_data))
			return;

		// todo abort if not needed anymore, query lock?

		std::shared_ptr<FiletypeCallback> filetype_callback = read_data->filetype_callback.lock();
		if (!debug_assert(filetype_callback))
			return;

		debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadInit>, read_data->file));
		event.set();

		filetype_callback->loadcall(std::move(stream), filetype_callback->data, read_data->file);

		debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadDone>, read_data->file));
		event.set();
	}
}

namespace engine
{
	namespace file
	{
		loader::~loader()
		{
			active.store(0, std::memory_order_relaxed);
			event.set();

			thread.join();

			::module_filesystem = nullptr;
		}

		loader::loader(system & system)
		{
			if (!debug_assert(::module_filesystem == nullptr))
				return;

			::module_filesystem = &system;

			active.store(1, std::memory_order_relaxed);
			thread = core::async::Thread{file_loader};
		}

		void register_library(loader & /*loader*/, engine::Asset directory)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageRegisterLibrary>, directory));
			event.set();

#if MODE_DEBUG
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags::RECURSE_DIRECTORIES;
#endif
			engine::file::scan(*::module_filesystem, directory, file_scan, utility::any(), mode);
		}

		void unregister_library(loader & /*loader*/, engine::Asset directory)
		{
#if MODE_DEBUG
			engine::file::remove_watch(*::module_filesystem, directory);
#endif

			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnregisterLibrary>, directory));
			event.set();
		}

		void register_filetype(loader & /*loader*/, engine::Asset filetype, load_callback * loadcall, unload_callback * unloadcall, utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageRegisterFiletype>, filetype, loadcall, unloadcall, std::move(data)));
			event.set();
		}

		void unregister_filetype(loader & /*loader*/, engine::Asset filetype)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnregisterFiletype>, filetype));
			event.set();
		}

		void load_global(
			loader & /*loader*/,
			engine::Asset filetype,
			engine::Asset file,
			ready_callback * readycall,
			unready_callback * unreadycall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadGlobal>, filetype, file, readycall, unreadycall, std::move(data)));
			event.set();
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
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadLocal>, filetype, owner, file, readycall, unreadycall, std::move(data)));
			event.set();
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
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadDependency>, filetype, owner, file, readycall, unreadycall, std::move(data)));
			event.set();
		}

		void unload_global(
			loader & /*loader*/,
			engine::Asset file)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnloadGlobal>, file));
			event.set();
		}

		void unload_local(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnloadLocal>, owner, file));
			event.set();
		}

		void unload_dependency(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnloadDependency>, owner, file));
			event.set();
		}
	}
}
