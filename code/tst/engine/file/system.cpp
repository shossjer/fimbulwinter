#include "engine/file/system.hpp"

#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"

#include <catch/catch.hpp>

#include <array>

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
}

TEST_CASE("file system can be created and destroyed", "[engine][file]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::file::system filesystem;
	}
}

#if !FILE_USE_DUMMY

TEST_CASE("file system can read files", "[engine][file]")
{
	engine::file::system filesystem;
	engine::file::register_temporary_directory(engine::Asset("tmpdir"));

	SECTION("that are created before the read starts")
	{
		struct SyncData
		{
			int counts[3] = {};
			int neither = 0;
			core::sync::Event<true> events[3];
		} sync_data;

		engine::file::write(engine::Asset("tmpdir"), u8"maybe.exists", write_char, utility::any(char(1)));
		engine::file::write(engine::Asset("tmpdir"), u8"maybe.whatever", write_char, utility::any(char(2)));
		engine::file::write(engine::Asset("tmpdir"), u8"whatever.exists", write_char, utility::any(char(4)));

		engine::file::read(
			engine::Asset("tmpdir"),
			u8"maybe.exists|maybe*|*.exists",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("maybe.exists"):
					sync_data.counts[0] += int(read_char(stream));
					sync_data.events[0].set();
					break;
				case engine::Asset("maybe"):
					sync_data.counts[1] += int(read_char(stream));
					sync_data.events[1].set();
					break;
				case engine::Asset(".exists"):
					sync_data.counts[2] += int(read_char(stream));
					sync_data.events[2].set();
					break;
				default:
					sync_data.neither = -100;
					sync_data.events[0].set();
					sync_data.events[1].set();
					sync_data.events[2].set();
				}
			},
			utility::any(&sync_data));

		// todo wait with timeout
		sync_data.events[0].wait();
		sync_data.events[1].wait();
		sync_data.events[2].wait();

		REQUIRE(sync_data.neither == 0);

		CHECK(sync_data.counts[0] == 1);
		CHECK(sync_data.counts[1] == 2);
		CHECK(sync_data.counts[2] == 4);
	}

	engine::file::unregister_directory(engine::Asset("tmpdir"));
}

TEST_CASE("file system can watch files", "[engine][file]")
{
	engine::file::system filesystem;
	engine::file::register_temporary_directory(engine::Asset("tmpdir"));

	SECTION("that are created both before and after the watch starts")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(engine::Asset("tmpdir"), u8"file.tmp", write_char, utility::any(char(1)));

		engine::file::watch(
			engine::Asset("tmpdir"),
			u8"file.tmp|file*|*.tmp",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("file.tmp"):
				case engine::Asset("file"):
				case engine::Asset(".tmp"):
					sync_data.count += int(read_char(stream));
				break;
				default:
					sync_data.count = -100;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data));

		// todo wait with timeout
		sync_data.event.wait();
		REQUIRE(sync_data.count == 1);
		sync_data.event.reset();

		engine::file::write(engine::Asset("tmpdir"), u8"file.whatever", write_char, utility::any(char(2)));

		// todo wait with timeout
		sync_data.event.wait();
		REQUIRE(sync_data.count == 3);
		sync_data.event.reset();

		engine::file::write(engine::Asset("tmpdir"), u8"whatever.tmp", write_char, utility::any(char(4)));

		// todo wait with timeout
		sync_data.event.wait();
		REQUIRE(sync_data.count == 7);
	}

	engine::file::unregister_directory(engine::Asset("tmpdir"));
}

#endif
