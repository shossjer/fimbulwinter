#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine//file/config.hpp"
#include "engine/file/scoped_directory.hpp"
#include "engine/file/system.hpp"
#include "engine/HashTable.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"

#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"

#include <catch2/catch.hpp>

static_hashes("tmpdir", "my read", "my scan", "strand");

namespace
{
	struct scoped_watch
	{
		engine::file::system & filesystem;
		engine::Token id;

		~scoped_watch()
		{
			engine::file::remove_watch(filesystem, id);
		}

		explicit scoped_watch(engine::file::system & filesystem, engine::Token id)
			: filesystem(filesystem)
			, id(id)
		{}
	};

	ext::ssize write_char(engine::file::system & /*filesystem*/, core::content & content, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<char>()))
			return 0;

		if (content.size() != 0)
		{
			const char number = utility::any_cast<char>(data);
			return ful::memcopy(&number + 0, &number + 1, static_cast<char *>(content.data())) - static_cast<char *>(content.data());
		}
		else
		{
			return 0;
		}
	};

	char read_char(core::content & content, int index = 0)
	{
		if (static_cast<ext::usize>(index) < content.size())
			return *(static_cast<char *>(content.data()) + index);
		return static_cast<char>(-1);
	}

	const int timeout = 1000; // milliseconds
}

TEST_CASE("file system can be created and destroyed", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);

	for (int i = 0; i < 2; i++)
	{
		engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	}
}

#if !FILE_USE_DUMMY

TEST_CASE("file system can read files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	engine::file::scoped_directory tmpdir(filesystem, engine::Hash("tmpdir"));

	SECTION("that are created before the read starts")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data[2];

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("maybe.exists"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		ful::assign(filepath1, ful::cstr_utf8("maybe.exists"));
		engine::file::read(
			filesystem,
			engine::Token{},
			tmpdir,
			std::move(filepath1),
			engine::Hash{},
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(content.filepath());
				sync_data.value = content.filepath() == u8"maybe.exists" ? int(read_char(content)) : -1;
				sync_data.event.set();
			},
			utility::any(sync_data + 0));
		ful::assign(filepath2, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::read(
			filesystem,
			engine::Token{},
			tmpdir,
			std::move(filepath2),
			engine::Hash{},
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(content.filepath());
				sync_data.value = content.filepath() == u8"folder/maybe.exists" ? int(read_char(content)) : -1;
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
			int value_3 = 0;
			int value_3_maybe = 0;
			int value_7 = 0;
			core::sync::Event<true> event_3;
			core::sync::Event<true> event_3_maybe;
			core::sync::Event<true> event_7;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash("strand"), write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		ful::assign(filepath1, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::read(
			filesystem,
			engine::Token(engine::Hash("my read")),
			tmpdir,
			std::move(filepath1),
			engine::Hash("strand"),
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			const auto value = int(read_char(content));
			if (value == 3)
			{
				if (sync_data.value_3 == 0)
				{
					sync_data.value_3 = value;
					sync_data.event_3.set();
				}
				else
				{
					sync_data.value_3_maybe = value;
					sync_data.event_3_maybe.set();
				}
			}
			else if (value == 7)
			{
				sync_data.value_7 = value;
				sync_data.event_7.set();
			}
		},
			utility::any(&sync_data),
			engine::file::flags::ADD_WATCH);

		scoped_watch watch(filesystem, engine::Token(engine::Hash("my read")));

		REQUIRE(sync_data.event_3.wait(timeout));
		CHECK(sync_data.value_3 == 3);

		ful::assign(filepath1, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash("strand"), write_char, utility::any(char(7)), engine::file::flags::OVERWRITE_EXISTING);

		REQUIRE(sync_data.event_7.wait(timeout));
		CHECK(sync_data.value_7 == 7);
		CHECK((sync_data.value_3_maybe == 0 || sync_data.value_3_maybe == 3));
	}
}

TEST_CASE("file system can scan directories", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	engine::file::scoped_directory tmpdir(filesystem, engine::Hash("tmpdir"));

	SECTION("that are empty")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::scan(
			filesystem,
			engine::Token{},
			tmpdir,
			engine::Hash{},
			[](engine::file::system & /*filesystem*/, engine::Hash directory, ful::heap_string_utf8 && existing_files, ful::heap_string_utf8 && /*removed_files*/, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			if (directory == engine::Hash("tmpdir"))
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

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("file.whatever"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::scan(
			filesystem,
			engine::Token{},
			tmpdir,
			engine::Hash{},
			[](engine::file::system & /*filesystem*/, engine::Hash directory, ful::heap_string_utf8 && existing_files, ful::heap_string_utf8 && /*removed_files*/, utility::any & data)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			auto & sync_data = *utility::any_cast<SyncData *>(data);

			if (directory == engine::Hash("tmpdir"))
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
			engine::Token(engine::Hash("my scan")),
			tmpdir,
			engine::Hash{},
			[](engine::file::system & /*filesystem*/, engine::Hash directory, ful::heap_string_utf8 && existing_files, ful::heap_string_utf8 && /*removed_files*/, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				debug_printline(existing_files);
				if (directory == engine::Hash("tmpdir"))
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

		scoped_watch watch(filesystem, engine::Token(engine::Hash("my scan")));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 1);
		sync_data.event.reset();

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("file.whatever"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash{}, write_char, utility::any(char(2)));

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 2);
		sync_data.event.reset();

		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		REQUIRE(sync_data.event.wait(timeout));
		REQUIRE(sync_data.count == 3);
	}
}

TEST_CASE("file system can write files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	engine::file::scoped_directory tmpdir(filesystem, engine::Hash("tmpdir"));

	SECTION("and ignore superfluous writes")
	{
		struct SyncData
		{
			int value = 0;
			core::sync::Event<true> event;
		} sync_data;

		// note strand must be set in order for the reads and writes not to
		// collide, since they all operate on the same file

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("new.file"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash("strand"), write_char, utility::any(char(3)));

		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		engine::file::read(
			filesystem,
			engine::Token{},
			tmpdir,
			std::move(filepath1),
			engine::Hash("strand"),
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = content.filepath() == u8"new.file" ? int(read_char(content)) + int(read_char(content, 1)) : -1;
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

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("new.file"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash("strand"), write_char, utility::any(char(3)), engine::file::flags::OVERWRITE_EXISTING);

		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		engine::file::read(
			filesystem,
			engine::Token{},
			tmpdir,
			std::move(filepath1),
			engine::Hash("strand"),
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = content.filepath() == u8"new.file" ? int(read_char(content)) + int(read_char(content, 1)) : -1;
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

		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("new.file"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash("strand"), write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash("strand"), write_char, utility::any(char(3)), engine::file::flags::APPEND_EXISTING);

		ful::assign(filepath1, ful::cstr_utf8("new.file"));
		engine::file::read(
			filesystem,
			engine::Token{},
			tmpdir,
			std::move(filepath1),
			engine::Hash("strand"),
			[](engine::file::system & /*filesystem*/, core::content & content, utility::any & data)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				auto & sync_data = *utility::any_cast<SyncData *>(data);

				sync_data.value = content.filepath() == u8"new.file" ? int(read_char(content)) + int(read_char(content, 1)) + int(read_char(content, 2)) : -1;
				sync_data.event.set();
			},
			utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.value == 5 - 1);
	}
}

#endif
