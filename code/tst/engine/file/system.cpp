#include "core/debug.hpp"
#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/scoped_directory.hpp"
#include "engine/file/system.hpp"
#include "engine/HashTable.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"

#include <catch2/catch.hpp>

static_hashes("tmpdir", "my read", "my scan", "strand");

namespace
{
	struct scoped_watch
	{
		engine::file::system & filesystem;
		engine::Identity id;

		~scoped_watch()
		{
			engine::file::remove_watch(filesystem, id);
		}

		explicit scoped_watch(engine::file::system & filesystem, engine::Identity id)
			: filesystem(filesystem)
			, id(id)
		{}
	};

	void write_char(engine::file::system & /*filesystem*/, core::WriteStream && stream, utility::any && data)
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
	engine::task::scheduler taskscheduler(1);

	for (int i = 0; i < 2; i++)
	{
		engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	}
}

#if !FILE_USE_DUMMY

TEST_CASE("file system can read files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("that are created before the read starts")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data[2];

		engine::file::write(filesystem, tmpdir, u8"maybe.exists", engine::Asset{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::read(
			filesystem,
			engine::Identity{},
			tmpdir,
			u8"maybe.exists",
			engine::Asset{},
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(stream.filepath());
				sync_data.value = stream.filepath() == u8"maybe.exists" ? int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(sync_data + 0));
		engine::file::read(
			filesystem,
			engine::Identity{},
			tmpdir,
			u8"folder/maybe.exists",
			engine::Asset{},
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(stream.filepath());
				sync_data.value = stream.filepath() == u8"folder/maybe.exists" ? int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(sync_data + 1));

		REQUIRE(sync_data[0].event.wait(timeout));
		REQUIRE(sync_data[1].event.wait(timeout));
		CHECK(sync_data[0].value == 2);
		CHECK(sync_data[1].value == 3);
	}

	SECTION("and watch later changes")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset("strand"), write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::read(
			filesystem,
			engine::Asset("my read"),
			tmpdir,
			u8"folder/maybe.exists",
			engine::Asset("strand"),
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			debug_printline(stream.filepath());
			sync_data.value += stream.filepath() == u8"folder/maybe.exists" ? int(read_char(stream)) : -1;
			sync_data.event.set();
		},
			utility::any(&sync_data),
			engine::file::flags::ADD_WATCH);

		scoped_watch watch(filesystem, engine::Asset("my read"));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 3);
		sync_data.event.reset();

		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset("strand"), write_char, utility::any(char(7)), engine::file::flags::OVERWRITE_EXISTING);

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 3 + 7);
		sync_data.event.reset();

		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset("strand"), write_char, utility::any(char(5)), engine::file::flags::OVERWRITE_EXISTING);

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 3 + 7 + 5);
	}
}

TEST_CASE("file system can scan directories", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("that are empty")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::scan(
			filesystem,
			engine::Identity{},
			tmpdir,
			engine::Asset{},
			[](engine::file::system & /*filesystem*/, engine::Asset directory, utility::heap_string_utf8 && existing_files, utility::heap_string_utf8 && /*removed_files*/, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			if (directory == engine::Asset("tmpdir"))
			{
				if (existing_files == u8"")
				{
					sync_data.count = 1;
				}
				else
				{
					sync_data.count = -1;
				}
			}
			else
			{
				sync_data.count = -1;
			}
			sync_data.event.set();
		},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
	}

	SECTION("and subdirectories")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(filesystem, tmpdir, u8"file.whatever", engine::Asset{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::scan(
			filesystem,
			engine::Identity{},
			tmpdir,
			engine::Asset{},
			[](engine::file::system & /*filesystem*/, engine::Asset directory, utility::heap_string_utf8 && existing_files, utility::heap_string_utf8 && /*removed_files*/, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			if (directory == engine::Asset("tmpdir"))
			{
				if (existing_files == u8"file.whatever;folder/maybe.exists" || existing_files == u8"folder/maybe.exists;file.whatever")
				{
					sync_data.count = 1;
				}
				else
				{
					sync_data.count = -1;
				}
			}
			else
			{
				sync_data.count = -1;
			}
			sync_data.event.set();
		},
			utility::any(&sync_data),
			engine::file::flags::RECURSE_DIRECTORIES);

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
	}

	SECTION("and watch file changes")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::scan(
			filesystem,
			engine::Asset("my scan"),
			tmpdir,
			engine::Asset{},
			[](engine::file::system & /*filesystem*/, engine::Asset directory, utility::heap_string_utf8 && existing_files, utility::heap_string_utf8 && /*removed_files*/, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(existing_files);
				if (directory == engine::Asset("tmpdir"))
				{
					if (existing_files == u8"")
					{
						sync_data.count = 1;
					}
					else if (existing_files == u8"file.whatever")
					{
						sync_data.count = 2;
					}
					else if (existing_files == u8"file.whatever;folder/maybe.exists" || existing_files == u8"folder/maybe.exists;file.whatever")
					{
						sync_data.count = 3;
					}
					else
					{
						sync_data.count = -1;
					}
				}
				else
				{
					sync_data.count = -1;
				}
				sync_data.event.set();
			},
			utility::any(&sync_data),
			engine::file::flags::RECURSE_DIRECTORIES | engine::file::flags::ADD_WATCH);

		scoped_watch watch(filesystem, engine::Asset("my scan"));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
		sync_data.event.reset();

		engine::file::write(filesystem, tmpdir, u8"file.whatever", engine::Asset{}, write_char, utility::any(char(2)));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 2);
		sync_data.event.reset();

		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 3);
	}
}

TEST_CASE("file system can write files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("and ignore superfluous writes")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(3)));

		engine::file::read(
			filesystem,
			engine::Identity{},
			tmpdir,
			u8"new.file",
			engine::Asset("strand"),
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = stream.filepath() == u8"new.file" ? int(read_char(stream)) + int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 2 - 1);
	}

	SECTION("and overwrite those that already exist with the `OVERWRITE_EXISTING` flag")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(3)), engine::file::flags::OVERWRITE_EXISTING);

		engine::file::read(
			filesystem,
			engine::Identity{},
			tmpdir,
			u8"new.file",
			engine::Asset("strand"),
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = stream.filepath() == u8"new.file" ? int(read_char(stream)) + int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 3 - 1);
	}

	SECTION("and append to files with the `APPEND_EXISTING` flag")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", engine::Asset("strand"), write_char, utility::any(char(3)), engine::file::flags::APPEND_EXISTING);

		engine::file::read(
			filesystem,
			engine::Identity{},
			tmpdir,
			u8"new.file",
			engine::Asset("strand"),
			[](engine::file::system & /*filesystem*/, core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = stream.filepath() == u8"new.file" ? int(read_char(stream)) + int(read_char(stream)) + int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 5 - 1);
	}
}

#endif
