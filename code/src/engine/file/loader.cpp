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
	struct ReadData
	{
		engine::Asset file;
		engine::file::loading_callback * loadingcall;
		utility::any & data; // todo this forces data to be stored in a unique_ptr

		explicit ReadData(engine::Asset file, engine::file::loading_callback * loadingcall, utility::any & data)
			: file(file)
			, loadingcall(loadingcall)
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
		engine::Asset directory;
		utility::heap_string_utf8 files;
	};

	struct MessageLoadGlobal
	{
		engine::Asset file;
		engine::file::loading_callback * loadingcall;
		engine::file::loaded_callback * loadedcall;
		engine::file::unload_callback * unloadcall;
		utility::any data;
	};

	struct MessageLoadLocal
	{
		engine::Asset owner;
		engine::Asset file;
		engine::file::loading_callback * loadingcall;
		engine::file::loaded_callback * loadedcall;
		engine::file::unload_callback * unloadcall;
		utility::any data;
	};

	struct MessageLoadDependency
	{
		engine::Asset owner;
		engine::Asset file;
		engine::file::loading_callback * loadingcall;
		engine::file::loaded_callback * loadedcall;
		engine::file::unload_callback * unloadcall;
		utility::any data;
	};

	struct MessageLoadDone
	{
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
		MessageLoadDone,
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
		engine::file::loading_callback * loadingcall;
		utility::heap_vector<engine::Asset, engine::file::loaded_callback *, engine::file::unload_callback *> calls;
		std::unique_ptr<utility::any> data; // todo

		engine::Asset directory;
		utility::heap_string_utf8 filepath;

		utility::heap_vector<engine::Asset> owners;
		utility::heap_vector<engine::Asset> attachments;

		std::int32_t dependency_count; // the number of attachments that are required
		std::int32_t done_count; // the number of loaded dependencies (the most significant bit is set if the file itself is not yet finished reading)

		explicit LoadingFile(engine::file::loading_callback * loadingcall, utility::any && data, engine::Asset directory, utility::heap_string_utf8 && filepath)
			: loadingcall(loadingcall)
			, data(std::make_unique<utility::any>(std::move(data))) // todo
			, directory(directory)
			, filepath(std::move(filepath))
			, dependency_count(0)
			, done_count(INT_MIN)
		{}
	};

	struct LoadedFile
	{
		engine::file::loading_callback * loadingcall;
		utility::heap_vector<engine::Asset, engine::file::loaded_callback *, engine::file::unload_callback *> calls;
		utility::any data;

		engine::Asset directory;
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
		engine::file::loading_callback * loadingcall,
		engine::file::loaded_callback * loadedcall,
		engine::file::unload_callback * unloadcall,
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
			KnownFile copy = std::move(y);

			// todo replace
			files.erase(file_it);
			LoadingFile * const loading_file = files.emplace<LoadingFile>(file, loadingcall, std::move(data), copy.directory, std::move(copy.filepath));
			if (!debug_verify(loading_file))
				return false; // error

			if (!debug_verify(loading_file->calls.try_emplace_back(actual_file, loadedcall, unloadcall)))
			{
				files.erase(find(files, file));
				return false; // error
			}

			if (!debug_verify(loading_file->owners.try_emplace_back(owner)))
			{
				files.erase(find(files, file));
				return false; // error
			}

#if MODE_DEBUG
			const auto mode = engine::file::flags::ADD_WATCH;
#else
			const auto mode = engine::file::flags{};
#endif
			engine::file::read(*::module_filesystem, loading_file->directory, utility::heap_string_utf8(loading_file->filepath), ReadData::file_load, ReadData(file, loading_file->loadingcall, *loading_file->data), mode);

			return true;
		},
			[&](LoadingFile & y)
		{
			if (!debug_verify(y.calls.try_emplace_back(actual_file, loadedcall, unloadcall)))
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
			if (!debug_verify(y.calls.try_emplace_back(actual_file, loadedcall, unloadcall)))
				return false;

			if (!debug_verify(y.owners.try_emplace_back(owner)))
			{
				ext::pop_back(y.calls);
				return false; // error
			}

			loadedcall(y.data, actual_file);

			utility::heap_vector<engine::Asset> relations;
			if (!debug_verify(relations.try_reserve(1)))
				return false; // error
			relations.try_emplace_back(utility::no_failure, owner);

			for (ext::index from = 0; static_cast<ext::usize>(from) != relations.size(); from++)
			{
				const auto owner_it = find(files, relations[from]);
				if (owner_it == files.end())
				{
					if (debug_assert(relations[from] == global))
						continue;

					return debug_fail();
				}

				if (LoadingFile * const loading_owner = files.get<LoadingFile>(owner_it))
				{
					loading_owner->done_count++;
					if (loading_owner->dependency_count == loading_owner->done_count)
					{
						if (!debug_verify(relations.try_reserve(relations.size() + loading_owner->owners.size())))
							return false; // error
						relations.push_back(utility::no_failure, loading_owner->owners.begin(), loading_owner->owners.end());

						if (!promote_loading_to_loaded(relations[from], owner_it, std::move(*loading_owner)))
							return false; // error
					}
				}
			}

			return true;
		},
			[&](UniqueFile & /*y*/) -> bool
		{
			debug_unreachable("radicals should not point to other radicals");
		}));
	}

	bool promote_unknown_to_known(engine::Asset file, decltype(files.end()) file_it, engine::Asset directory, utility::heap_string_utf8 && filepath)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		return debug_verify(files.emplace<KnownFile>(file, directory, std::move(filepath)));
	}

	bool promote_loading_to_loaded(engine::Asset file, decltype(files.end()) file_it, LoadingFile loading_file)
	{
		// todo replace
		files.erase(file_it);
		// todo on failure the file is lost
		const auto loaded_file = files.emplace<LoadedFile>(file, loading_file.loadingcall, std::move(loading_file.calls), std::move(*loading_file.data), loading_file.directory, std::move(loading_file.filepath), std::move(loading_file.owners), std::move(loading_file.attachments));
		if (!debug_verify(loaded_file))
			return false;

		for (auto && call : loaded_file->calls)
		{
			std::get<1>(call)(loaded_file->data, std::get<0>(call));
		}

		return true;
	}

	void remove_file(
		decltype(files.end()) file_it,
		engine::Asset file,
		utility::heap_vector<engine::Asset> && attachments,
		const utility::heap_vector<engine::Asset, engine::file::loaded_callback *, engine::file::unload_callback *> & calls,
		utility::any && data)
	{
		utility::heap_vector<engine::Asset, engine::Asset> dependencies;
		if (!debug_verify(dependencies.try_reserve(attachments.size())))
			return; // error
		for (auto attachment : attachments)
		{
			dependencies.try_emplace_back(utility::no_failure, file, attachment);
		}

		for (auto && call : calls)
		{
			std::get<2>(call)(data, std::get<0>(call));
		}

		files.erase(file_it);

		while (!ext::empty(dependencies))
		{
			const utility::heap_vector<engine::Asset, engine::Asset>::value_type dependency = ext::back(std::move(dependencies));
			ext::pop_back(dependencies);

			const auto attachment_it = find(files, dependency.second);
			if (!debug_assert(attachment_it != files.end()))
				break;

			files.call(attachment_it, ext::overload(
				[&](AmbiguousFile & /*x*/)
			{
				debug_unreachable();
			},
				[&](DirectoryFile & /*x*/)
			{
				debug_unreachable();
			},
				[&](KnownFile & /*x*/)
			{
				debug_unreachable();
			},
				[&](LoadingFile & x)
			{
				const auto owner_it = ext::find(x.owners, dependency.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath));
#endif

					if (!debug_verify(dependencies.try_reserve(dependencies.size() + x.attachments.size())))
						return; // error
					for (auto attachment : x.attachments)
					{
						dependencies.try_emplace_back(utility::no_failure, dependency.second, attachment);
					}

					for (auto && call : x.calls)
					{
						// todo the loading callback can be called as the same time as the unload callback, is this a problem?
						std::get<2>(call)(*x.data, std::get<0>(call));
					}

					files.erase(attachment_it);
				}
			},
				[&](LoadedFile & x)
			{
				const auto owner_it = ext::find(x.owners, dependency.first);
				if (!debug_assert(owner_it != x.owners.end()))
					return;

				x.owners.erase(owner_it);

				if (ext::empty(x.owners))
				{
#if MODE_DEBUG
					engine::file::remove_watch(*::module_filesystem, x.directory, std::move(x.filepath));
#endif

					if (!debug_verify(dependencies.try_reserve(dependencies.size() + x.attachments.size())))
						return; // error
					for (auto attachment : x.attachments)
					{
						dependencies.try_emplace_back(utility::no_failure, dependency.second, attachment);
					}

					for (auto && call : x.calls)
					{
						std::get<2>(call)(x.data, std::get<0>(call));
					}

					files.erase(attachment_it);
				}
			},
				[&](UniqueFile & /*x*/)
			{
				debug_unreachable();
			}));
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
									[&](AmbiguousFile & /*y*/)
								{
									if (!debug_assert(find(begin, split, u8'.') == split))
										return;

									files.erase(file_it);

									debug_printline(file_asset, " is known: ", filepath);
									if (!debug_verify(files.emplace<KnownFile>(file_asset, x.directory, utility::heap_string_utf8(filepath))))
										return; // error
								},
									[&](DirectoryFile & /*y*/)
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
									[&](UniqueFile & /*y*/) // todo same as AmbiguousFile
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
											[&](DirectoryFile & /*y*/) {},
											[&](KnownFile & /*y*/) {},
											[&](LoadingFile & /*y*/) {},
											[&](LoadedFile & /*y*/) {},
											[&](UniqueFile & /*y*/)
										{
											files.erase(radical_it);
										}));
									}
								}

								files.call(file_it, ext::overload(
									[&](AmbiguousFile & /*y*/)
								{
									debug_fail(file_asset, " is ambigous?");
								},
									[&](DirectoryFile & /*y*/)
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
									[&](UniqueFile & /*y*/) // todo same as AmbiguousFile
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

				if (!load_file<ReadData>(global, underlying_file.first, x.file, underlying_file.second, x.loadingcall, x.loadedcall, x.unloadcall, std::move(x.data)))
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

				if (!load_file<ReadData>(underlying_owner.first, underlying_file.first, x.file, underlying_file.second, x.loadingcall, x.loadedcall, x.unloadcall, std::move(x.data)))
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

				if (!load_file<ReadData>(underlying_owner.first, underlying_file.first, x.file, underlying_file.second, x.loadingcall, x.loadedcall, x.unloadcall, std::move(x.data)))
					return; // error
				// todo undo on failure
			}

			void operator () (MessageLoadDone && x)
			{
				const auto file_it = find(files, x.file);
				LoadingFile * const loading_file = files.get<LoadingFile>(file_it);
				if (!debug_assert(loading_file))
					return; // error

				loading_file->done_count &= INT32_MAX;
				if (loading_file->dependency_count == loading_file->done_count)
				{
					utility::heap_vector<engine::Asset> relations;
					if (!debug_verify(relations.try_reserve(loading_file->owners.size())))
						return; // error
					relations.push_back(utility::no_failure, loading_file->owners.begin(), loading_file->owners.end());

					if (!promote_loading_to_loaded(x.file, file_it, std::move(*loading_file)))
						return; // error

					for (ext::index from = 0; static_cast<ext::usize>(from) != relations.size(); from++)
					{
						const auto owner_it = find(files, relations[from]);
						if (owner_it == files.end())
						{
							if (debug_assert(relations[from] == global))
								continue;

							return;
						}

						if (LoadingFile * const loading_owner = files.get<LoadingFile>(owner_it))
						{
							loading_owner->done_count++;
							if (loading_owner->dependency_count == loading_owner->done_count)
							{
								if (!debug_verify(relations.try_reserve(relations.size() + loading_owner->owners.size())))
									return; // error
								relations.push_back(utility::no_failure, loading_owner->owners.begin(), loading_owner->owners.end());

								if (!promote_loading_to_loaded(relations[from], owner_it, std::move(*loading_owner)))
									return; // error
							}
						}
					}
				}
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(*y.data));
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(y.data));
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(*y.data));
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(y.data));
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(*y.data));
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
#if MODE_DEBUG
						engine::file::remove_watch(*::module_filesystem, y.directory, std::move(y.filepath));
#endif

						remove_file(underlying_file.second, underlying_file.first, std::move(y.attachments), y.calls, std::move(y.data));
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

		read_data->loadingcall(std::move(stream), read_data->data, read_data->file);

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

		void load_global(
			loader & /*loader*/,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadGlobal>, file, loadingcall, loadedcall, unloadcall, std::move(data)));
			event.set();
		}

		void load_local(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadLocal>, owner, file, loadingcall, loadedcall, unloadcall, std::move(data)));
			event.set();
		}

		void load_dependency(
			loader & /*loader*/,
			engine::Asset owner,
			engine::Asset file,
			loading_callback * loadingcall,
			loaded_callback * loadedcall,
			unload_callback * unloadcall,
			utility::any && data)
		{
			debug_verify(queue.try_emplace(utility::in_place_type<MessageLoadDependency>, owner, file, loadingcall, loadedcall, unloadcall, std::move(data)));
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
