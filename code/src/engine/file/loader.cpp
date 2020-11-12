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
	struct ReadDataGlobal
	{
		engine::Asset file;
		engine::Asset actual_file;
		engine::file::load_callback * readcall;
		utility::any & data;

		explicit ReadDataGlobal(engine::Asset /*owner*/, engine::Asset file, engine::Asset actual_file, engine::file::load_callback * readcall, utility::any & data)
			: file(file)
			, actual_file(actual_file)
			, readcall(readcall)
			, data(data)
		{}

		static void file_load(core::ReadStream && stream, utility::any & data);
	};

	struct ReadDataLocal
	{
		engine::Asset file;
		engine::Asset actual_file;
		engine::file::load_callback * readcall;
		utility::any & data;

		explicit ReadDataLocal(engine::Asset /*owner*/, engine::Asset file, engine::Asset actual_file, engine::file::load_callback * readcall, utility::any & data)
			: file(file)
			, actual_file(actual_file)
			, readcall(readcall)
			, data(data)
		{}

		static void file_load(core::ReadStream && stream, utility::any & data);
	};

	struct ReadDataDependency
	{
		engine::Asset owner;
		engine::Asset file;
		engine::Asset actual_file;
		engine::file::load_callback * readcall;
		utility::any & data;

		explicit ReadDataDependency(engine::Asset owner, engine::Asset file, engine::Asset actual_file, engine::file::load_callback * readcall, utility::any & data)
			: owner(owner)
			, file(file)
			, actual_file(actual_file)
			, readcall(readcall)
			, data(data)
		{}

		static void file_load(core::ReadStream && stream, utility::any & data);
	};

	constexpr auto global = engine::Asset{};
}

namespace
{
	struct MessageFileScan
	{
		engine::Asset dictionary;
		utility::heap_string_utf8 files;
	};

	struct MessageLoadGlobal
	{
		engine::Asset file;
		engine::file::load_callback * readcall;
		engine::file::purge_callback * purgecall;
		utility::any data;
	};

	struct MessageLoadLocal
	{
		engine::Asset owner;
		engine::Asset file;
		engine::file::load_callback * readcall;
		engine::file::purge_callback * purgecall;
		utility::any data;
	};

	struct MessageLoadDependency
	{
		engine::Asset owner;
		engine::Asset file;
		engine::file::load_callback * readcall;
		engine::file::purge_callback * purgecall;
		utility::any data;
	};

	struct MessageLoadGlobalDone
	{
		engine::Asset file;
	};

	struct MessageLoadLocalDone
	{
		engine::Asset file;
	};

	struct MessageLoadDependencyDone
	{
		engine::Asset owner;
		engine::Asset file;
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
		MessageLoadGlobalDone,
		MessageLoadLocalDone,
		MessageLoadDependencyDone,
		MessageRegisterLibrary,
		MessageUnloadGlobal,
		MessageUnloadLocal,
		MessageUnloadDependency,
		MessageUnregisterLibrary
	>;

	core::container::PageQueue<utility::heap_storage<Message>> queue;
}

namespace
{
	engine::file::system * module_filesystem = nullptr;

	struct AmbiguousFile
	{
	};

	struct DirectoryFile
	{
	};

	struct KnownFile
	{
		engine::Asset dictionary;
		utility::heap_string_utf8 filepath;
	};

	struct LoadingFile
	{
		engine::file::load_callback * readcall;
		engine::file::purge_callback * purgecall;
		std::unique_ptr<utility::any> data; // todo

		engine::Asset dictionary;
		utility::heap_string_utf8 filepath;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		std::int32_t dependency_count; // the number of attachments that are required
		std::int32_t done_count; // the number of loaded dependencies (the most significant bit is set if the file itself is not yet finished reading)
	};

	struct LoadedFile
	{
		engine::file::load_callback * readcall;
		engine::file::purge_callback * purgecall;
		utility::any data;

		engine::Asset dictionary;
		utility::heap_string_utf8 filepath;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;
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

	template <typename ReadData>
	bool load_file(
		engine::Asset owner,
		engine::Asset file,
		engine::Asset actual_file,
		decltype(files.end()) file_it,
		engine::file::load_callback * readcall,
		engine::file::purge_callback * purgecall,
		utility::any && data)
	{
		return files.call(file_it, ext::overload(
			[](AmbiguousFile & /*y*/)
		{
			return debug_fail("cannot load ambiguous files");
		},
			[](DirectoryFile & /*y*/)
		{
			return debug_fail("cannot load directory files");
		},
			[&](KnownFile & y)
		{
			utility::heap_vector<engine::Asset> owners;
			if (!debug_verify(owners.try_emplace_back(owner)))
				return false; // error

			utility::heap_vector<engine::Asset> attachments;

			KnownFile copy = std::move(y);

			// todo replace
			files.erase(file_it);
			LoadingFile * const loading_file = files.emplace<LoadingFile>(file, readcall, purgecall, std::make_unique<utility::any>(std::move(data)), copy.dictionary, std::move(copy.filepath), std::move(owners), std::move(attachments), 0, INT_MIN);
			if (!debug_verify(loading_file))
				return false; // error

#if MODE_DEBUG
			const auto mode = engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags{};
#endif
			engine::file::read(*::module_filesystem, loading_file->dictionary, utility::heap_string_utf8(loading_file->filepath), ReadData::file_load, ReadData(owner, file, actual_file, loading_file->readcall, *loading_file->data), mode);

			return true;
		},
			[&](LoadingFile & y)
		{
			return debug_verify(y.owners.try_emplace_back(owner));
		},
			[&](LoadedFile & y)
		{
			return debug_verify(y.owners.try_emplace_back(owner));
		},
			[&](UniqueFile & /*y*/) -> bool
		{
			debug_unreachable("radicals should not point to other radicals");
		}));
	}

	bool promote_unknown_to_known(engine::Asset file, decltype(files.end()) file_it, engine::Asset dictionary, utility::heap_string_utf8 && filepath)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		return debug_verify(files.emplace<KnownFile>(file, dictionary, std::move(filepath)));
	}

	bool promote_loading_to_loaded(engine::Asset file, decltype(files.end()) file_it, LoadingFile loading_file)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		return debug_verify(files.emplace<LoadedFile>(file, loading_file.readcall, loading_file.purgecall, std::move(*loading_file.data), loading_file.dictionary, std::move(loading_file.filepath), std::move(loading_file.owners), std::move(loading_file.attachments)));
	}

	bool finish_loading(engine::Asset file, decltype(files.end()) file_it)
	{
		LoadingFile * const loading_file = files.get<LoadingFile>(file_it);
		if (!debug_assert(loading_file))
			return false;

		loading_file->done_count &= INT32_MAX;
		if (loading_file->dependency_count == loading_file->done_count)
		{
			if (!promote_loading_to_loaded(file, file_it, std::move(*loading_file)))
				return false; // error
		}
		return true;
	}

	void remove_loading_file(
		decltype(files.end()) file_it,
		engine::Asset file,
		utility::heap_vector<engine::Asset> && attachments,
		engine::file::purge_callback * purgecall,
		utility::any && data)
	{
		struct Blob
		{
			engine::Asset file;
			utility::heap_vector<engine::Asset> attachments;
		};
		utility::heap_vector<Blob> blobs;
		if (!debug_verify(blobs.try_emplace_back(file, std::move(attachments))))
			return; // error

		purgecall(std::move(data), file);

		files.erase(file_it);

		while (!ext::empty(blobs))
		{
			const auto blob = ext::back(std::move(blobs));
			ext::pop_back(blobs);

			for (engine::Asset dependency : blob.attachments)
			{
				const auto dependency_it = find(files, dependency);
				if (!debug_assert(dependency_it != files.end()))
					break;

				if (LoadedFile * const loaded_dependency_ptr = files.get<LoadedFile>(dependency_it))
				{
					const auto owner_it = ext::find(loaded_dependency_ptr->owners, blob.file);
					if (!debug_assert(owner_it != loaded_dependency_ptr->owners.end()))
						break;

					loaded_dependency_ptr->owners.erase(owner_it);

					if (ext::empty(loaded_dependency_ptr->owners))
					{
						if (!debug_verify(blobs.try_emplace_back(dependency, std::move(loaded_dependency_ptr->attachments))))
							return; // error
					}
				}
				else if (LoadingFile * const loading_dependency_ptr = files.get<LoadingFile>(dependency_it))
				{
					const auto owner_it = ext::find(loading_dependency_ptr->owners, blob.file);
					if (!debug_assert(owner_it != loading_dependency_ptr->owners.end()))
						break;

					loading_dependency_ptr->owners.erase(owner_it);

					if (ext::empty(loading_dependency_ptr->owners))
					{
						if (!debug_verify(blobs.try_emplace_back(dependency, std::move(loading_dependency_ptr->attachments))))
							return; // error
					}
				}
				else
				{
					debug_fail();
				}
			}
		}
	}

	void remove_loaded_file(
		decltype(files.end()) file_it,
		engine::Asset file,
		utility::heap_vector<engine::Asset> && attachments,
		engine::file::purge_callback * purgecall,
		utility::any && data)
	{
		struct Blob
		{
			engine::Asset file;
			utility::heap_vector<engine::Asset> attachments;
		};
		utility::heap_vector<Blob> blobs;
		if (!debug_verify(blobs.try_emplace_back(file, std::move(attachments))))
			return; // error

		purgecall(std::move(data), file);

		files.erase(file_it);

		while (!ext::empty(blobs))
		{
			const auto blob = ext::back(std::move(blobs));
			ext::pop_back(blobs);

			for (engine::Asset dependency : blob.attachments)
			{
				const auto dependency_it = find(files, dependency);
				if (!debug_assert(dependency_it != files.end()))
					break;

				LoadedFile * const dependency_ptr = files.get<LoadedFile>(dependency_it);
				if (!debug_assert(dependency_ptr))
					break;

				const auto owner_it = ext::find(dependency_ptr->owners, blob.file);
				if (!debug_assert(owner_it != dependency_ptr->owners.end()))
					break;

				dependency_ptr->owners.erase(owner_it);

				if (ext::empty(dependency_ptr->owners))
				{
					if (!debug_verify(blobs.try_emplace_back(dependency, std::move(dependency_ptr->attachments))))
						return; // error
				}
			}
		}
	}

	engine::Asset scanning_dictionary{};
	utility::heap_vector<Message> delayed_messages;

	void process_message(Message && message)
	{
		struct ProcessMessage
		{
			void operator () (MessageFileScan && x)
			{
				scanning_dictionary = engine::Asset{};

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

						const auto file_it = find(files, file_asset);
						if (file_it != files.end())
						{
							files.call(file_it, ext::overload(
								[&](AmbiguousFile & /*y*/)
							{
								if (!debug_assert(find(begin, split, u8'.') == split))
									return;

								files.erase(file_it);

								debug_printline(file_asset, " is known: ", filepath);
								if (!debug_verify(files.emplace<KnownFile>(file_asset, x.dictionary, utility::heap_string_utf8(filepath))))
									return; // error
							},
								[&](DirectoryFile & /*y*/)
							{
								debug_unreachable(file_asset, " has previously been registered as a directory");
							},
								[&](KnownFile & y)
							{
								debug_assert(y.dictionary == x.dictionary) && debug_assert(y.filepath == filepath);
							},
								[&](LoadingFile & y)
							{
								debug_assert(y.dictionary == x.dictionary) && debug_assert(y.filepath == filepath);
							},
								[&](LoadedFile & y)
							{
								debug_assert(y.dictionary == x.dictionary) && debug_assert(y.filepath == filepath);
							},
								[&](UniqueFile & /*y*/) // todo same as AmbiguousFile
							{
								if (!debug_assert(find(begin, split, u8'.') == split))
									return;

								files.erase(file_it);

								debug_printline(file_asset, " is known: ", filepath);
								if (!debug_verify(files.emplace<KnownFile>(file_asset, x.dictionary, utility::heap_string_utf8(filepath))))
									return; // error
							}));
						}
						else
						{
							debug_printline(file_asset, " is known: ", filepath);
							if (!debug_verify(files.emplace<KnownFile>(file_asset, x.dictionary, utility::heap_string_utf8(filepath))))
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
										[&](AmbiguousFile & /*y*/)
									{
									},
										[&](DirectoryFile & /*y*/) {},
										[&](KnownFile & /*y*/) {},
										[&](LoadingFile & /*y*/) {},
										[&](LoadedFile & /*y*/) {},
										[&](UniqueFile & /*y*/)
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

				if (!load_file<ReadDataGlobal>(global, underlying_file.first, x.file, underlying_file.second, x.readcall, x.purgecall, std::move(x.data)))
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
					[&](AmbiguousFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile & /*y*/)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile & /*y*/)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					return debug_verify(y.attachments.try_emplace_back(underlying_file.first));
				},
					[&](LoadedFile & y)
				{
					return debug_verify(y.attachments.try_emplace_back(underlying_file.first));
				},
					[&](UniqueFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return; // error

				if (!load_file<ReadDataLocal>(underlying_owner.first, underlying_file.first, x.file, underlying_file.second, x.readcall, x.purgecall, std::move(x.data)))
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
					[&](AmbiguousFile & /*y*/) -> bool
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
					if (debug_verify(y.attachments.insert(y.attachments.begin() + y.dependency_count, underlying_file.first)))
					{
						y.dependency_count++;

						return true;
					}
					return false;
				},
					[&](LoadedFile &)
				{
					return debug_fail("file ", underlying_owner.first, " is already loaded and cannot depend on further files");
				},
					[&](UniqueFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return; // error

				if (!load_file<ReadDataDependency>(underlying_owner.first, underlying_file.first, x.file, underlying_file.second, x.readcall, x.purgecall, std::move(x.data)))
					return; // error
				// todo undo on failure
			}

			void operator () (MessageLoadGlobalDone && x)
			{
				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				if (!finish_loading(underlying_file.first, underlying_file.second))
					return; // error
			}

			void operator () (MessageLoadLocalDone && x)
			{
				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				if (!finish_loading(underlying_file.first, underlying_file.second))
					return; // error
			}

			void operator () (MessageLoadDependencyDone && x)
			{
				const auto underlying_file = find_underlying_file(x.file);
				if (!debug_verify(underlying_file.second != files.end()))
					return; // error

				if (!finish_loading(underlying_file.first, underlying_file.second))
					return; // error

				const auto owner_it = find(files, x.owner);
				if (!debug_assert(owner_it != files.end()))
					return;

				LoadingFile * const loading_owner = files.get<LoadingFile>(owner_it);
				if (!debug_assert(loading_owner))
					return;

				loading_owner->done_count++;
				if (loading_owner->dependency_count == loading_owner->done_count)
				{
					if (!promote_loading_to_loaded(x.owner, owner_it, std::move(*loading_owner)))
						return; // error
				}
			}

			void operator () (MessageRegisterLibrary && x)
			{
				scanning_dictionary = x.directory;

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

				files.call(underlying_file.second, ext::overload(
					[&](AmbiguousFile & /*y*/)
				{
					debug_unreachable("file ", underlying_file.first, " is ambiguous");
				},
					[&](DirectoryFile &)
				{
					if (!debug_fail("file ", underlying_file.first, " is a directory and cannot be unloaded"))
						return; // error
				},
					[&](KnownFile &)
				{
					if (!debug_fail("cannot unload already unloaded file ", underlying_file.first))
						return; // error
				},
					[&](LoadingFile & y)
				{
					const auto owner_it = ext::find(y.owners, global);
					if (!debug_verify(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return; // error

					if (ext::empty(y.owners))
					{
						remove_loading_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(*y.data));
					}
				},
					[&](LoadedFile & y)
				{
					const auto owner_it = ext::find(y.owners, global);
					if (!debug_verify(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return; // error

					y.owners.erase(owner_it);

					if (ext::empty(y.owners))
					{
						remove_loaded_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(y.data));
					}
				},
					[&](UniqueFile & /*y*/)
				{
					debug_unreachable("radicals should not point to other radicals");
				}));
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
					[&](AmbiguousFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				},
					[&](DirectoryFile & /*y*/)
				{
					return debug_fail("file ", underlying_owner.first, " is a directory and cannot be an owner");
				},
					[&](KnownFile & /*y*/)
				{
					return debug_fail("file ", underlying_owner.first, " is unloaded and cannot be an owner");
				},
					[&](LoadingFile & y)
				{
					const auto file_it = ext::find(y.attachments, underlying_file.first);
					if (!debug_verify(file_it != y.attachments.end()))
						return false; // error

					if (!debug_assert(y.dependency_count <= file_it - y.attachments.begin()))
						return false;

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
					[&](UniqueFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return;

				files.call(underlying_file.second, ext::overload(
					[&](AmbiguousFile & /*y*/)
				{
					debug_unreachable("file ", underlying_file.first, " is ambiguous");
				},
					[&](DirectoryFile &)
				{
					if (!debug_fail("file ", underlying_file.first, " is a directory and cannot be unloaded"))
						return; // error
				},
					[&](KnownFile &)
				{
					if (!debug_fail("cannot unload already unloaded file ", underlying_file.first))
						return; // error
				},
					[&](LoadingFile & y)
				{
					const auto owner_it = ext::find(y.owners, underlying_owner.first);
					if (!debug_assert(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return;

					if (ext::empty(y.owners))
					{
						remove_loading_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(*y.data));
					}
				},
					[&](LoadedFile & y)
				{
					const auto owner_it = ext::find(y.owners, underlying_owner.first);
					if (!debug_assert(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return;

					y.owners.erase(owner_it);

					if (ext::empty(y.owners))
					{
						remove_loaded_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(y.data));
					}
				},
					[&](UniqueFile & /*y*/)
				{
					debug_unreachable("radicals should not point to other radicals");
				}));
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
					[&](AmbiguousFile & /*y*/) -> bool
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

					if (!debug_assert(file_it - y.attachments.begin() < y.dependency_count))
						return false;

					y.attachments.erase(file_it);
					y.dependency_count--;
					if (y.dependency_count == y.done_count)
					{
						if (!promote_loading_to_loaded(underlying_owner.first, underlying_owner.second, std::move(y)))
							return false; // error
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
					[&](UniqueFile & /*y*/) -> bool
				{
					debug_unreachable("file ", underlying_owner.first, " is a radical and cannot be an owner");
				}));
				if (!success)
					return;

				files.call(underlying_file.second, ext::overload(
					[&](AmbiguousFile & /*y*/)
				{
					debug_unreachable("file ", underlying_file.first, " is ambiguous");
				},
					[&](DirectoryFile &)
				{
					if (!debug_fail("file ", underlying_file.first, " is a directory and cannot be unloaded"))
						return; // error
				},
					[&](KnownFile &)
				{
					if (!debug_fail("cannot unload already unloaded file ", underlying_file.first))
						return; // error
				},
					[&](LoadingFile & y)
				{
					const auto owner_it = ext::find(y.owners, global);
					if (!debug_verify(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return; // error

					if (ext::empty(y.owners))
					{
						remove_loading_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(*y.data));
					}
				},
					[&](LoadedFile & y)
				{
					const auto owner_it = ext::find(y.owners, global);
					if (!debug_verify(owner_it != y.owners.end(), "file ", underlying_file.first, " is not global"))
						return; // error

					y.owners.erase(owner_it);

					if (ext::empty(y.owners))
					{
						remove_loaded_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.purgecall, std::move(y.data));
					}
				},
					[&](UniqueFile & /*y*/)
				{
					debug_unreachable("radicals should not point to other radicals");
				}));
			}

			void operator () (MessageUnregisterLibrary && /*x*/)
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
			if (scanning_dictionary != engine::Asset{})
			{
				if (utility::holds_alternative<MessageFileScan>(message) && utility::get<MessageFileScan>(message).dictionary == scanning_dictionary)
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
								if (utility::holds_alternative<MessageFileScan>(*it) && utility::get<MessageFileScan>(*it).dictionary == scanning_dictionary)
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

	void ReadDataGlobal::file_load(core::ReadStream && stream, utility::any & data)
	{
		ReadDataGlobal * const read_data = utility::any_cast<ReadDataGlobal>(&data);
		if (!debug_assert(read_data))
			return;

		// todo abort if not needed anymore, query lock?

		read_data->readcall(std::move(stream), read_data->data, read_data->actual_file);

		debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadGlobalDone>, read_data->file));
		event.set();
	}

	void ReadDataLocal::file_load(core::ReadStream && stream, utility::any & data)
	{
		ReadDataLocal * const read_data = utility::any_cast<ReadDataLocal>(&data);
		if (!debug_assert(read_data))
			return;

		// todo abort if not needed anymore, query lock?

		read_data->readcall(std::move(stream), read_data->data, read_data->actual_file);

		debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadLocalDone>, read_data->file));
		event.set();
	}

	void ReadDataDependency::file_load(core::ReadStream && stream, utility::any & data)
	{
		ReadDataDependency * const read_data = utility::any_cast<ReadDataDependency>(&data);
		if (!debug_assert(read_data))
			return;

		// todo abort if not needed anymore, query lock?

		read_data->readcall(std::move(stream), read_data->data, read_data->actual_file);

		debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadDependencyDone>, read_data->owner, read_data->file));
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
			debug_verify(queue.try_emplace(utility::in_place_type<MessageUnregisterLibrary>, directory));
			event.set();

#if MODE_DEBUG
			//engine::file::stop_watch(directory);
#endif
		}

		void load_global(
			loader & /*loader*/,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadGlobal>, file, readcall, purgecall, std::move(data)));
			event.set();
		}

		void load_local(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadLocal>, owner, file, readcall, purgecall, std::move(data)));
			event.set();
		}

		void load_dependency(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file,
			load_callback * readcall,
			purge_callback * purgecall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadDependency>, owner, file, readcall, purgecall, std::move(data)));
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
