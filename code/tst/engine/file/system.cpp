#include "engine/file/system.hpp"

#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "utility/any.hpp"

#include <catch/catch.hpp>

#include <array>

debug_assets("tmpdir");

TEST_CASE("file system can be created and destroyed", "[engine][file]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::file::system filesystem;
	}
}

#if !FILE_USE_DUMMY

TEST_CASE("file system can create temporary directories", "[engine][file]")
{
	engine::file::system filesystem;

	engine::file::register_temporary_directory(engine::Asset("tmpdir"));

	auto write_char = [](core::WriteStream && stream, utility::any && data)
	                  {
		                  if (!debug_assert(data.type_id() == utility::type_id<char>()))
			                  return;

		                  const char number = utility::any_cast<char>(data);
		                  stream.write_all(&number, sizeof number);
	                  };

	SECTION("and read files created within")
	{
		const char numbers[] = {7, 5, 3};

		SECTION("on temporary files created within")
		{
			engine::file::write(engine::Asset("tmpdir"), u8"maybe.exists", write_char, utility::any(numbers[0]));
			engine::file::write(engine::Asset("tmpdir"), u8"maybe.whatever", write_char, utility::any(numbers[1]));
			engine::file::write(engine::Asset("tmpdir"), u8"whatever.exists", write_char, utility::any(numbers[2]));
		}

		SECTION("even if they do not exist")
		{
		}

		struct SyncData
		{
			int counts[3] = {};
			int neither = 0;
			core::sync::Event<true> events[3];
		} sync_data;

		engine::file::read(engine::Asset("tmpdir"),
		                   u8"maybe.exists|maybe*|*.exists",
		                   [](core::ReadStream && stream, utility::any & data, engine::Asset match)
		                   {
			                   if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				                   return;

			                   auto & sync_data = *utility::any_cast<SyncData *>(data);

			                   auto read_number = [](core::ReadStream & stream)
			                                      {
				                                      char number = 0;
				                                      stream.read_all(&number, sizeof number);
				                                      return int(number);
			                                      };

			                   switch (match)
			                   {
			                   case engine::Asset("maybe.exists"):
				                   sync_data.counts[0] += read_number(stream);
				                   sync_data.events[0].set();
				                   break;
			                   case engine::Asset("maybe"):
				                   sync_data.counts[1] += read_number(stream);
				                   sync_data.events[1].set();
				                   break;
			                   case engine::Asset(".exists"):
				                   sync_data.counts[2] += read_number(stream);
				                   sync_data.events[2].set();
				                   break;
			                   case engine::Asset(""):
				                   sync_data.counts[0] += 7; // todo numbers[0]
				                   sync_data.counts[1] += 5; // todo numbers[1]
				                   sync_data.counts[2] += 3; // todo numbers[2]
				                   sync_data.events[0].set();
				                   sync_data.events[1].set();
				                   sync_data.events[2].set();
				                   break;
			                   default:
				                   sync_data.neither = -1;
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

		CHECK(sync_data.counts[0] == int(numbers[0]));
		CHECK(sync_data.counts[1] == int(numbers[1]));
		CHECK(sync_data.counts[2] == int(numbers[2]));
	}

	SECTION("and watch for file changes")
	{
		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::watch(engine::Asset("tmpdir"),
		                    u8"file.tmp|file*|*.tmp",
		                    [](core::ReadStream && stream, utility::any & data, engine::Asset match)
		                    {
			                    if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				                    return;

			                    auto & sync_data = *utility::any_cast<SyncData *>(data);

			                    char number = 0;
			                    stream.read_all(&number, sizeof number);

			                    switch (match)
			                    {
			                    case engine::Asset("file.tmp"):
				                    sync_data.count += int(number);
				                    sync_data.event.set();
				                    break;
			                    case engine::Asset("file"):
				                    sync_data.count += int(number);
				                    sync_data.event.set();
				                    break;
			                    case engine::Asset(".tmp"):
				                    sync_data.count += int(number);
				                    sync_data.event.set();
				                    break;
			                    default:
				                    sync_data.event.set();
			                    }
		                    },
		                    utility::any(&sync_data));

		SECTION("on temporary files created within")
		{
			engine::file::write(engine::Asset("tmpdir"), u8"file.tmp", write_char, utility::any(char(1)));

			// todo wait with timeout
			sync_data.event.wait();
			sync_data.event.reset();

			CHECK(sync_data.count == 1);

			engine::file::write(engine::Asset("tmpdir"), u8"file.whatever", write_char, utility::any(char(2)));

			// todo wait with timeout
			sync_data.event.wait();
			sync_data.event.reset();

			CHECK(sync_data.count == 3);

			engine::file::write(engine::Asset("tmpdir"), u8"whatever.tmp", write_char, utility::any(char(4)));

			// todo wait with timeout
			sync_data.event.wait();

			CHECK(sync_data.count == 7);
		}
	}

	SECTION("and watch will find already existing files")
	{
		const char number = 11;

		engine::file::write(engine::Asset("tmpdir"), u8"already.existing", write_char, utility::any(number));

		struct SyncData
		{
			int count = 0;
			core::sync::Event<true> event;
		} sync_data;

		engine::file::watch(engine::Asset("tmpdir"),
		                    u8"already.existing",
		                    [](core::ReadStream && stream, utility::any & data, engine::Asset match)
		                    {
			                    if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				                    return;

			                    auto & sync_data = *utility::any_cast<SyncData *>(data);

			                    char number = 0;
			                    stream.read_all(&number, sizeof number);

			                    switch (match)
			                    {
			                    case engine::Asset("already.existing"):
				                    sync_data.count += int(number);
				                    sync_data.event.set();
				                    break;
			                    default:
				                    sync_data.event.set();
			                    }
		                    },
		                    utility::any(&sync_data));

		// todo wait with timeout
		sync_data.event.wait();

		CHECK(sync_data.count == int(number));
	}

	engine::file::unregister_directory(engine::Asset("tmpdir"));
}

#endif
