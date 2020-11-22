#include "engine/file/loader.hpp"

#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/scoped_directory.hpp"
#include "engine/file/scoped_library.hpp"
#include "engine/file/system.hpp"

#include "utility/any.hpp"

#include <catch2/catch.hpp>

debug_assets("tmpdir", "tree.root", "dependency.1", "dependency.2", "dependency.3", "dependency.4", "dependency.5");

namespace
{
	void write_char(core::WriteStream && stream, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<char>()))
			return;

		const char number = utility::any_cast<char>(data);
		stream.write_all(&number, sizeof number);
	};

	char read_char(core::ReadStream & stream)
	{
		char number = -1;
		stream.read_all(&number, sizeof number);
		return number;
	}

	const int timeout = 1000; // milliseconds
}

TEST_CASE("file loader can be created and destroyed", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());

	for (int i = 0; i < 2; i++)
	{
		engine::file::loader fileloader(filesystem);
	}
}

TEST_CASE("file loader can read files", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());
	engine::file::loader fileloader(filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("")
	{
		engine::file::write(filesystem, tmpdir, u8"maybe.exists", write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data[2];

		engine::file::load_global(
			fileloader,
			engine::Asset(u8"maybe.exists"),
			[](core::ReadStream && stream, utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.value = file == engine::Asset(u8"maybe.exists") ? int(read_char(stream)) : -1;
		},
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.event.set();
		},
			[](utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.value += file == engine::Asset(u8"maybe.exists") ? 1 : -1;
			sync_data.event.set();
		},
			utility::any(sync_data + 0));

		engine::file::load_global(
			fileloader,
			engine::Asset(u8"folder/maybe.exists"),
			[](core::ReadStream && stream, utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.value = file == engine::Asset(u8"folder/maybe.exists") ? int(read_char(stream)) : -1;
		},
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.event.set();
		},
			[](utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			sync_data.value += file == engine::Asset(u8"folder/maybe.exists") ? 1 : -1;
			sync_data.event.set();
		},
			utility::any(sync_data + 1));

		REQUIRE(sync_data[0].event.wait(timeout));
		REQUIRE(sync_data[1].event.wait(timeout));
		CHECK(sync_data[0].value == 2);
		CHECK(sync_data[1].value == 3);

		sync_data[0].event.reset();
		sync_data[1].event.reset();

		engine::file::unload_global(fileloader, engine::Asset(u8"maybe.exists"));
		engine::file::unload_global(fileloader, engine::Asset(u8"folder/maybe.exists"));

		REQUIRE(sync_data[0].event.wait(timeout));
		REQUIRE(sync_data[1].event.wait(timeout));
		CHECK(sync_data[0].value == 3);
		CHECK(sync_data[1].value == 4);
	}
}

TEST_CASE("file loader can load tree", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());
	engine::file::loader fileloader(filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("")
	{
		engine::file::write(filesystem, tmpdir, u8"tree.root", write_char, utility::any(char(1)));
		engine::file::write(filesystem, tmpdir, u8"dependency.1", write_char, utility::any(char(11)));
		engine::file::write(filesystem, tmpdir, u8"dependency.2", write_char, utility::any(char(12)));
		engine::file::write(filesystem, tmpdir, u8"dependency.3", write_char, utility::any(char(13)));
		engine::file::write(filesystem, tmpdir, u8"dependency.4", write_char, utility::any(char(14)));
		engine::file::write(filesystem, tmpdir, u8"dependency.5", write_char, utility::any(char(15)));

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		struct SyncData
		{
			engine::file::loader & fileloader;

			int loadings[6] = {};
			int counts[6] = {};
			core::sync::Event<true> event[8];

			static void loading(core::ReadStream && stream, utility::any & data, engine::Asset file)
			{
				debug_printline("loading ", file);

				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.1"), loading, loaded, unload, utility::any(data));
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.2"), loading, loaded, unload, utility::any(data));
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.3"), loading, loaded, unload, utility::any(data));
					sync_data.loadings[0] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.1"):
					sync_data.loadings[1] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.2"):
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.3"), loading, loaded, unload, utility::any(data));
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.4"), loading, loaded, unload, utility::any(data));
					sync_data.loadings[2] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.3"):
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.1"), loading, loaded, unload, utility::any(data));
					sync_data.loadings[3] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.4"):
					engine::file::load_dependency(sync_data.fileloader, file, engine::Asset(u8"dependency.5"), loading, loaded, unload, utility::any(data));
					sync_data.loadings[4] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.5"):
					sync_data.loadings[5] += int(read_char(stream));
					break;
				default:
					debug_unreachable();
				}
			}

			static void loaded(utility::any & data, engine::Asset file)
			{
				debug_printline("loaded ", file);

				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					sync_data.counts[0]++;
					if (debug_verify(sync_data.counts[0] == 1))
					{
						sync_data.event[0].set();
					}
					break;
				case engine::Asset(u8"dependency.1"):
					sync_data.counts[1]++;
					if (sync_data.counts[1] == 1)
					{
						sync_data.event[1].set();
					}
					else if (debug_verify(sync_data.counts[1] == 2))
					{
						sync_data.event[2].set();
					}
					break;
				case engine::Asset(u8"dependency.2"):
					sync_data.counts[2]++;
					if (debug_verify(sync_data.counts[2] == 1))
					{
						sync_data.event[3].set();
					}
					break;
				case engine::Asset(u8"dependency.3"):
					sync_data.counts[3]++;
					if (sync_data.counts[3] == 1)
					{
						sync_data.event[4].set();
					}
					else if (debug_verify(sync_data.counts[3] == 2))
					{
						sync_data.event[5].set();
					}
					break;
				case engine::Asset(u8"dependency.4"):
					sync_data.counts[4]++;
					if (debug_verify(sync_data.counts[4] == 1))
					{
						sync_data.event[6].set();
					}
					break;
				case engine::Asset(u8"dependency.5"):
					sync_data.counts[5]++;
					if (debug_verify(sync_data.counts[5] == 1))
					{
						sync_data.event[7].set();
					}
					break;
				default:
					debug_unreachable();
				}
			}

			static void unload(utility::any & data, engine::Asset file)
			{
				debug_printline("unloading ", file);

				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					sync_data.counts[0]++;
					if (debug_verify(sync_data.counts[0] == 1))
					{
						sync_data.event[0].set();
					}
					break;
				case engine::Asset(u8"dependency.1"):
					sync_data.counts[1]++;
					if (sync_data.counts[1] == 1)
					{
						sync_data.event[1].set();
					}
					else if (debug_verify(sync_data.counts[1] == 2))
					{
						sync_data.event[2].set();
					}
					break;
				case engine::Asset(u8"dependency.2"):
					sync_data.counts[2]++;
					if (debug_verify(sync_data.counts[2] == 1))
					{
						sync_data.event[3].set();
					}
					break;
				case engine::Asset(u8"dependency.3"):
					sync_data.counts[3]++;
					if (sync_data.counts[3] == 1)
					{
						sync_data.event[4].set();
					}
					else if (debug_verify(sync_data.counts[3] == 2))
					{
						sync_data.event[5].set();
					}
					break;
				case engine::Asset(u8"dependency.4"):
					sync_data.counts[4]++;
					if (debug_verify(sync_data.counts[4] == 1))
					{
						sync_data.event[6].set();
					}
					break;
				case engine::Asset(u8"dependency.5"):
					sync_data.counts[5]++;
					if (debug_verify(sync_data.counts[5] == 1))
					{
						sync_data.event[7].set();
					}
					break;
				default:
					debug_unreachable();
				}
			}
		} sync_data{fileloader};

		engine::file::load_global(fileloader, engine::Asset(u8"tree.root"), SyncData::loading, SyncData::loaded, SyncData::unload, utility::any(&sync_data));

		REQUIRE(sync_data.event[0].wait(timeout));
		REQUIRE(sync_data.event[1].wait(timeout));
		REQUIRE(sync_data.event[2].wait(timeout));
		REQUIRE(sync_data.event[3].wait(timeout));
		REQUIRE(sync_data.event[4].wait(timeout));
		REQUIRE(sync_data.event[5].wait(timeout));
		REQUIRE(sync_data.event[6].wait(timeout));
		REQUIRE(sync_data.event[7].wait(timeout));
		CHECK(sync_data.loadings[0] == 1);
		CHECK(sync_data.loadings[1] == 11);
		CHECK(sync_data.loadings[2] == 12);
		CHECK(sync_data.loadings[3] == 13);
		CHECK(sync_data.loadings[4] == 14);
		CHECK(sync_data.loadings[5] == 15);
		CHECK(sync_data.counts[0] == 1);
		CHECK(sync_data.counts[1] == 2);
		CHECK(sync_data.counts[2] == 1);
		CHECK(sync_data.counts[3] == 2);
		CHECK(sync_data.counts[4] == 1);
		CHECK(sync_data.counts[5] == 1);
		sync_data.event[0].reset();
		sync_data.event[1].reset();
		sync_data.event[2].reset();
		sync_data.event[3].reset();
		sync_data.event[4].reset();
		sync_data.event[5].reset();
		sync_data.event[6].reset();
		sync_data.event[7].reset();
		sync_data.counts[0] = 0;
		sync_data.counts[1] = 0;
		sync_data.counts[2] = 0;
		sync_data.counts[3] = 0;
		sync_data.counts[4] = 0;
		sync_data.counts[5] = 0;

		engine::file::unload_global(fileloader, engine::Asset(u8"tree.root"));

		REQUIRE(sync_data.event[0].wait(timeout));
		REQUIRE(sync_data.event[1].wait(timeout));
		REQUIRE(sync_data.event[2].wait(timeout));
		REQUIRE(sync_data.event[3].wait(timeout));
		REQUIRE(sync_data.event[4].wait(timeout));
		REQUIRE(sync_data.event[5].wait(timeout));
		REQUIRE(sync_data.event[6].wait(timeout));
		REQUIRE(sync_data.event[7].wait(timeout));
		CHECK(sync_data.counts[0] == 1);
		CHECK(sync_data.counts[1] == 2);
		CHECK(sync_data.counts[2] == 1);
		CHECK(sync_data.counts[3] == 2);
		CHECK(sync_data.counts[4] == 1);
		CHECK(sync_data.counts[5] == 1);
	}
}
