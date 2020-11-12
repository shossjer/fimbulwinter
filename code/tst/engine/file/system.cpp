#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/scoped_directory.hpp"
#include "engine/file/system.hpp"
#include "engine/HashTable.hpp"

#include "utility/any.hpp"

#include <catch2/catch.hpp>

#include <array>

static_hashes("tmpdir");

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

TEST_CASE("file system can be created and destroyed", "[engine][file]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::file::system filesystem(engine::file::directory::working_directory());
	}
}

#if !FILE_USE_DUMMY

TEST_CASE("file system can read files", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("that are created before the read starts")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data[2];

		engine::file::write(filesystem, tmpdir, u8"maybe.exists", write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::read(
			filesystem,
			tmpdir,
			u8"maybe.exists",
			[](core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = stream.filepath() == u8"maybe.exists" ? int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(sync_data + 0));
		engine::file::read(
			filesystem,
			tmpdir,
			u8"folder/maybe.exists",
			[](core::ReadStream && stream, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = stream.filepath() == u8"folder/maybe.exists" ? int(read_char(stream)) : -1;
				sync_data.event.set();
			},
			utility::any(sync_data + 1));

		REQUIRE(sync_data[0].event.wait(timeout));
		REQUIRE(sync_data[1].event.wait(timeout));
		CHECK(sync_data[0].value == 2);
		CHECK(sync_data[1].value == 3);
	}
}

TEST_CASE("file system can watch files", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));
}

TEST_CASE("file system can write files", "[engine][file]")
{
	engine::file::system filesystem(engine::file::directory::working_directory());
	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("and ignore superfluous writes")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(3)));

		engine::file::read(
			filesystem,
			tmpdir,
			u8"new.file",
			[](core::ReadStream && stream, utility::any & data)
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

		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(3)), engine::file::flags::OVERWRITE_EXISTING);

		engine::file::read(
			filesystem,
			tmpdir,
			u8"new.file",
			[](core::ReadStream && stream, utility::any & data)
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

		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"new.file", write_char, utility::any(char(3)), engine::file::flags::APPEND_EXISTING);

		engine::file::read(
			filesystem,
			tmpdir,
			u8"new.file",
			[](core::ReadStream && stream, utility::any & data)
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
