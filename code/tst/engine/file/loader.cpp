#include "core/ReadStream.hpp"
#include "core/sync/Event.hpp"
#include "core/WriteStream.hpp"

#include "engine/file/loader.hpp"
#include "engine/file/scoped_directory.hpp"
#include "engine/file/scoped_library.hpp"
#include "engine/file/system.hpp"
#include "engine/task/scheduler.hpp"

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
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());

	for (int i = 0; i < 2; i++)
	{
		engine::file::loader fileloader(taskscheduler, filesystem);
	}
}

TEST_CASE("file loader can read files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	engine::file::loader fileloader(taskscheduler, filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("")
	{
		engine::file::write(filesystem, tmpdir, u8"maybe.exists", engine::Asset{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, u8"folder/maybe.exists", engine::Asset{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		struct FileData
		{
			int values[2] = {};
			core::sync::Event<true> load_events[2];
			core::sync::Event<true> unload_events[2];
		} file_data;

		engine::file::scoped_filetype filetype(
			fileloader,
			engine::Asset("tmpfiletype"),
			[](core::ReadStream && stream, utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<FileData *>()))
				return;

			FileData * const file_data = utility::any_cast<FileData *>(data);

			switch (file)
			{
			case engine::Asset(u8"maybe.exists"):
				file_data->values[0] += int(read_char(stream));
				file_data->load_events[0].set();
				break;
			case engine::Asset(u8"folder/maybe.exists"):
				file_data->values[1] += int(read_char(stream));
				file_data->load_events[1].set();
				break;
			default:
				debug_fail();
			}
		},
			[](utility::any & data, engine::Asset file)
		{
			if (!debug_assert(data.type_id() == utility::type_id<FileData *>()))
				return;

			FileData * const file_data = utility::any_cast<FileData *>(data);

			switch (file)
			{
			case engine::Asset(u8"maybe.exists"):
				file_data->values[0] -= 2;
				file_data->unload_events[0].set();
				break;
			case engine::Asset(u8"folder/maybe.exists"):
				file_data->values[1] -= 3;
				file_data->unload_events[1].set();
				break;
			default:
				debug_fail();
			}
		},
			&file_data);

		struct SyncData
		{
			core::sync::Event<true> ready_event;
			core::sync::Event<true> unready_event;
		} sync_data[2];

		engine::file::load_global(
			fileloader,
			filetype,
			engine::Asset(u8"maybe.exists"),
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);

			sync_data->ready_event.set();
		},
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);

			sync_data->unready_event.set();
		},
			utility::any(sync_data + 0));

		engine::file::load_global(
			fileloader,
			filetype,
			engine::Asset(u8"folder/maybe.exists"),
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);

			sync_data->ready_event.set();
		},
			[](utility::any & data, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);

			sync_data->unready_event.set();
		},
			utility::any(sync_data + 1));

		REQUIRE(file_data.load_events[0].wait(timeout));
		REQUIRE(file_data.load_events[1].wait(timeout));
		REQUIRE(sync_data[0].ready_event.wait(timeout));
		REQUIRE(sync_data[1].ready_event.wait(timeout));
		CHECK(file_data.values[0] == 2);
		CHECK(file_data.values[1] == 3);

		file_data.load_events[0].reset();
		file_data.load_events[1].reset();
		sync_data[0].ready_event.reset();
		sync_data[1].ready_event.reset();

		engine::file::unload_global(fileloader, engine::Asset(u8"maybe.exists"));
		engine::file::unload_global(fileloader, engine::Asset(u8"folder/maybe.exists"));

		REQUIRE(sync_data[0].unready_event.wait(timeout));
		REQUIRE(sync_data[1].unready_event.wait(timeout));
		REQUIRE(file_data.unload_events[0].wait(timeout));
		REQUIRE(file_data.unload_events[1].wait(timeout));
		CHECK(file_data.values[0] == 0);
		CHECK(file_data.values[1] == 0);
	}
}

TEST_CASE("file loader can load tree", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory());
	engine::file::loader fileloader(taskscheduler, filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("")
	{
		engine::file::write(filesystem, tmpdir, u8"tree.root", engine::Asset{}, write_char, utility::any(char(1)));
		engine::file::write(filesystem, tmpdir, u8"dependency.1", engine::Asset{}, write_char, utility::any(char(11)));
		engine::file::write(filesystem, tmpdir, u8"dependency.2", engine::Asset{}, write_char, utility::any(char(12)));
		engine::file::write(filesystem, tmpdir, u8"dependency.3", engine::Asset{}, write_char, utility::any(char(13)));
		engine::file::write(filesystem, tmpdir, u8"dependency.4", engine::Asset{}, write_char, utility::any(char(14)));
		engine::file::write(filesystem, tmpdir, u8"dependency.5", engine::Asset{}, write_char, utility::any(char(15)));

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		struct SyncData
		{
			int values[6] = {};
			core::sync::Event<true> event;
			core::sync::Event<true> event_dep2;
		} sync_data;

		struct FileData
		{
			engine::file::loader & fileloader;
			SyncData & sync_data;

			int values[6] = {};
			core::sync::Event<true> event;
		} file_data{fileloader, sync_data};

		struct Callbacks
		{
			static void load(core::ReadStream && stream, utility::any & data, engine::Asset file)
			{
				if (!debug_assert(data.type_id() == utility::type_id<FileData *>()))
					return;

				FileData * const file_data = utility::any_cast<FileData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.1"), ready, unready, &file_data->sync_data);
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.2"), ready, unready, &file_data->sync_data);
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.3"), ready, unready, &file_data->sync_data);
					file_data->values[0] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.1"):
					file_data->values[1] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.2"):
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.3"), ready, unready, &file_data->sync_data);
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.4"), ready, unready, &file_data->sync_data);
					file_data->values[2] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.3"):
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.1"), ready, unready, &file_data->sync_data);
					file_data->values[3] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.4"):
					engine::file::load_dependency(file_data->fileloader, engine::Asset("tmpfiletype"), file, engine::Asset(u8"dependency.5"), ready, unready, &file_data->sync_data);
					file_data->values[4] += int(read_char(stream));
					break;
				case engine::Asset(u8"dependency.5"):
					file_data->values[5] += int(read_char(stream));
					break;
				default:
					debug_fail();
				}
			}

			static void unload(utility::any & data, engine::Asset file)
			{
				if (!debug_assert(data.type_id() == utility::type_id<FileData *>()))
					return;

				FileData * const file_data = utility::any_cast<FileData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					file_data->values[0] -= 1;
					break;
				case engine::Asset(u8"dependency.1"):
					file_data->values[1] -= 11;
					break;
				case engine::Asset(u8"dependency.2"):
					file_data->values[2] -= 12;
					break;
				case engine::Asset(u8"dependency.3"):
					file_data->values[3] -= 13;
					break;
				case engine::Asset(u8"dependency.4"):
					file_data->values[4] -= 14;
					break;
				case engine::Asset(u8"dependency.5"):
					file_data->values[5] -= 15;
					break;
				default:
					debug_fail();
				}

				if (file_data->values[0] == 0 && file_data->values[1] == 0 && file_data->values[2] == 0 && file_data->values[3] == 0 && file_data->values[4] == 0 && file_data->values[5] == 0)
				{
					file_data->event.set();
				}
			}

			static void ready(utility::any & data, engine::Asset file)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				SyncData * const sync_data = utility::any_cast<SyncData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					sync_data->values[0]++;
					sync_data->event.set();
					break;
				case engine::Asset(u8"dependency.1"):
					sync_data->values[1]++;
					break;
				case engine::Asset(u8"dependency.2"):
					sync_data->values[2]++;
					sync_data->event_dep2.set();
					break;
				case engine::Asset(u8"dependency.3"):
					sync_data->values[3]++;
					break;
				case engine::Asset(u8"dependency.4"):
					sync_data->values[4]++;
					break;
				case engine::Asset(u8"dependency.5"):
					sync_data->values[5]++;
					break;
				default:
					debug_unreachable();
				}
			}

			static void unready(utility::any & data, engine::Asset file)
			{
				if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
					return;

				SyncData * const sync_data = utility::any_cast<SyncData *>(data);

				switch (file)
				{
				case engine::Asset(u8"tree.root"):
					sync_data->values[0]--;
					break;
				case engine::Asset(u8"dependency.1"):
					sync_data->values[1]--;
					break;
				case engine::Asset(u8"dependency.2"):
					sync_data->values[2]--;
					break;
				case engine::Asset(u8"dependency.3"):
					sync_data->values[3]--;
					break;
				case engine::Asset(u8"dependency.4"):
					sync_data->values[4]--;
					break;
				case engine::Asset(u8"dependency.5"):
					sync_data->values[5]--;
					break;
				default:
					debug_unreachable();
				}
			}
		};

		engine::file::scoped_filetype filetype(fileloader, engine::Asset("tmpfiletype"), Callbacks::load, Callbacks::unload, &file_data);

		engine::file::load_global(fileloader, filetype, engine::Asset(u8"tree.root"), Callbacks::ready, Callbacks::unready, utility::any(&sync_data));

		REQUIRE(sync_data.event.wait(timeout));
		CHECK(sync_data.values[0] == 1);
		CHECK(sync_data.values[1] == 2);
		CHECK(sync_data.values[2] == 1);
		CHECK(sync_data.values[3] == 2);
		CHECK(sync_data.values[4] == 1);
		CHECK(sync_data.values[5] == 1);
		CHECK(file_data.values[0] == 1);
		CHECK(file_data.values[1] == 11);
		CHECK(file_data.values[2] == 12);
		CHECK(file_data.values[3] == 13);
		CHECK(file_data.values[4] == 14);
		CHECK(file_data.values[5] == 15);

		file_data.event.reset();
		sync_data.event.reset();
		sync_data.event_dep2.reset();

		engine::file::write(filesystem, tmpdir, u8"dependency.2", engine::Asset{}, write_char, utility::any(char(21)), engine::file::flags::OVERWRITE_EXISTING);

		REQUIRE(sync_data.event_dep2.wait(timeout));
		CHECK(sync_data.values[2] == 1);
		CHECK(file_data.values[2] == 33);

		file_data.event.reset();
		sync_data.event.reset();
		sync_data.event_dep2.reset();
		file_data.values[2] = 12;

		engine::file::unload_global(fileloader, engine::Asset(u8"tree.root"));

		REQUIRE(file_data.event.wait(timeout));
		CHECK(sync_data.values[0] == 0);
		CHECK(sync_data.values[1] == 0);
		CHECK(sync_data.values[2] == 0);
		CHECK(sync_data.values[3] == 0);
		CHECK(sync_data.values[4] == 0);
		CHECK(sync_data.values[5] == 0);
		CHECK(file_data.values[0] == 0);
		CHECK(file_data.values[1] == 0);
		CHECK(file_data.values[2] == 0);
		CHECK(file_data.values[3] == 0);
		CHECK(file_data.values[4] == 0);
		CHECK(file_data.values[5] == 0);
	}
}
