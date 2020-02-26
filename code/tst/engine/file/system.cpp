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
	struct temporary_directory
	{
		engine::Asset directory;

		~temporary_directory()
		{
			engine::file::unregister_directory(directory);
		}

		temporary_directory(engine::Asset directory)
			: directory(directory)
		{
			engine::file::register_temporary_directory(directory);
		}

		operator engine::Asset () const { return directory; }
	};

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
	temporary_directory tmpdir = engine::Asset("tmpdir");

	SECTION("that are created before the read starts")
	{
		struct SyncData
		{
			int counts[3] = {};
			int neither = 0;
			core::sync::Event<true> events[3];
		} sync_data;

		engine::file::write(tmpdir, u8"maybe.exists", write_char, utility::any(char(1)));
		engine::file::write(tmpdir, u8"maybe.whatever", write_char, utility::any(char(2)));
		engine::file::write(tmpdir, u8"whatever.exists", write_char, utility::any(char(4)));

		engine::file::read(
			tmpdir,
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

		REQUIRE(sync_data.events[0].wait(timeout));
		REQUIRE(sync_data.events[1].wait(timeout));
		REQUIRE(sync_data.events[2].wait(timeout));

		REQUIRE(sync_data.neither == 0);

		CHECK(sync_data.counts[0] == 1);
		CHECK(sync_data.counts[1] == 2);
		CHECK(sync_data.counts[2] == 4);
	}

	SECTION("and be told when no matchings are possible with the `REPORT_MISSING` flag")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::read(
			tmpdir,
			u8"maybe.exists",
			[](core::ReadStream && /*stream*/, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset(""):
					sync_data.count += 1;
					break;
				default:
					sync_data.count = -100;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data),
			engine::file::flags::REPORT_MISSING);

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
	}

	SECTION("and only read the first match with the `ONCE_ONLY` flag")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(tmpdir, u8"maybe.exists", write_char, utility::any(char(1)));
		engine::file::write(tmpdir, u8"maybe.whatever", write_char, utility::any(char(1)));
		engine::file::write(tmpdir, u8"whatever.exists", write_char, utility::any(char(1)));

		engine::file::read(
			tmpdir,
			u8"maybe.exists|maybe*|*.exists",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("maybe.exists"):
				case engine::Asset("maybe"):
				case engine::Asset(".exists"):
					sync_data.count += int(read_char(stream));
					break;
				default:
					sync_data.count = -100;
				}
			},
			utility::any(&sync_data),
			engine::file::flags::ONCE_ONLY);

		engine::file::read(
			tmpdir,
			u8"maybe.exists",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("maybe.exists"):
					sync_data.count += int(read_char(stream));
					break;
				default:
					sync_data.count = -100;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 2);
	}
}

TEST_CASE("file system can watch files", "[engine][file]")
{
	engine::file::system filesystem;
	temporary_directory tmpdir = engine::Asset("tmpdir");

	SECTION("that are created both before and after the watch starts")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(tmpdir, u8"file.tmp", write_char, utility::any(char(1)));

		engine::file::watch(
			tmpdir,
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

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
		sync_data.event.reset();

		engine::file::write(tmpdir, u8"file.whatever", write_char, utility::any(char(2)));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 3);
		sync_data.event.reset();

		engine::file::write(tmpdir, u8"whatever.tmp", write_char, utility::any(char(4)));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 7);
	}

	SECTION("and be told when no matchings are possible with the `REPORT_MISSING` flag")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::watch(
			tmpdir,
			u8"maybe.exists",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset(""):
					sync_data.count += 1;
					break;
				case engine::Asset("maybe.exists"):
					sync_data.count += int(read_char(stream));
					break;
				default:
					sync_data.count = -100;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data),
			engine::file::flags::REPORT_MISSING);

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
		sync_data.event.reset();

		engine::file::write(tmpdir, u8"maybe.exists", write_char, utility::any(char(10)));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 11);
		sync_data.event.reset();

		engine::file::remove(tmpdir, u8"maybe.exists");

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 12);
	}

	SECTION("and ignore those that already exist with the `IGNORE_EXISTING` flag")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(tmpdir, u8"already.existing", write_char, utility::any(char(1)));

		engine::file::watch(
			tmpdir,
			u8"already.existing",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("already.existing"):
					sync_data.count += int(read_char(stream));
					break;
				default:
					sync_data.count = -100;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data),
			engine::file::flags::IGNORE_EXISTING);

		engine::file::write(
			tmpdir,
			u8"already.existing",
			[](core::WriteStream && stream, utility::any && data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.event.reset();

				const char number = 2;
				stream.write_all(&number, sizeof number);
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 2);
	}

	SECTION("and only watch the first match with the `ONCE_ONLY` flag")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> events[2];
		} sync_data;

		engine::file::write(tmpdir, u8"already.existing", write_char, utility::any(char(1)));
		engine::file::write(tmpdir, u8"already.whatever", write_char, utility::any(char(1)));
		engine::file::write(tmpdir, u8"whatever.existing", write_char, utility::any(char(1)));

		engine::file::watch(
			tmpdir,
			u8"already.existing",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("already.existing"):
					sync_data.count += int(read_char(stream));
					sync_data.events[0].set();
					break;
				default:
					sync_data.events[1].set();
				}
			},
			utility::any(&sync_data),
			engine::file::flags::ONCE_ONLY);

		REQUIRE(sync_data.events[0].wait(timeout));
		REQUIRE(!sync_data.events[1].wait(100));
		REQUIRE(sync_data.count == 1);
		sync_data.events[0].reset();

		engine::file::watch(
			tmpdir,
			u8"maybe.exists|maybe*|*.exists",
			[](core::ReadStream && stream, utility::any & data, engine::Asset match)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				switch (match)
				{
				case engine::Asset("maybe.exists"):
					sync_data.count += int(read_char(stream));
					sync_data.events[0].set();
					break;
				default:
					sync_data.events[1].set();
				}
			},
			utility::any(&sync_data),
			engine::file::flags::ONCE_ONLY);

		engine::file::write(tmpdir, u8"maybe.exists", write_char, utility::any(char(2)));
		engine::file::write(tmpdir, u8"maybe.whatever", write_char, utility::any(char(4)));
		engine::file::write(tmpdir, u8"whatever.exists", write_char, utility::any(char(8)));

		REQUIRE(sync_data.events[0].wait(timeout));
		REQUIRE(!sync_data.events[1].wait(100));
		REQUIRE(sync_data.count == 3);
	}
}

#endif
