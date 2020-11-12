#include "engine/file/loader.hpp"

#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/scoped_directory.hpp"
#include "engine/file/scoped_library.hpp"
#include "engine/file/system.hpp"

#include "utility/any.hpp"

#include <catch2/catch.hpp>

debug_assets("tmpdir");

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
			sync_data.event.set();
		},
			[](utility::any && data, engine::Asset file)
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
			sync_data.event.set();
		},
			[](utility::any && data, engine::Asset file)
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
