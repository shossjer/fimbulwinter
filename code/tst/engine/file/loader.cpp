#include "core/content.hpp"
#include "core/sync/Event.hpp"

#include "engine/file/config.hpp"
#include "engine/file/loader.hpp"
#include "engine/file/scoped_directory.hpp"
#include "engine/file/scoped_library.hpp"
#include "engine/file/system.hpp"
#include "engine/HashTable.hpp"
#include "engine/task/scheduler.hpp"

#include "utility/any.hpp"

#include "ful/string_init.hpp"
#include "ful/string_modify.hpp"

#include <catch2/catch.hpp>

static_hashes("tmpdir", "tree.root", "dependency.1", "dependency.2", "dependency.3", "dependency.4", "dependency.5");

namespace
{
	ext::ssize write_char(engine::file::system & /*filesystem*/, core::content & content, utility::any && data)
	{
		if (!debug_assert(data.type_id() == utility::type_id<char>()))
			return 0;

		const char number = utility::any_cast<char>(data);
		return ful::memcopy(&number + 0, &number + 1, static_cast<char *>(content.data())) - static_cast<char *>(content.data());
	};

	char read_char(core::content & content)
	{
		if (content.size() < 1)
			return static_cast<char>(-1);

		return *static_cast<char *>(content.data());
	}

	const int timeout = 1000; // milliseconds
}

TEST_CASE("file loader can be created and destroyed", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});

	for (int i = 0; i < 2; i++)
	{
		engine::file::loader fileloader(taskscheduler, filesystem);
	}
}

TEST_CASE("file loader can read files", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	engine::file::loader fileloader(taskscheduler, filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Hash("tmpdir"));

	SECTION("")
	{
		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("maybe.exists"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("folder/maybe.exists"));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Hash{}, write_char, utility::any(char(2)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Hash{}, write_char, utility::any(char(3)), engine::file::flags::CREATE_DIRECTORIES);

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		struct FileData
		{
			int value = {};

			int * unload_value = nullptr;
			core::sync::Event<true> * unload_event = nullptr;
		};

		engine::file::scoped_filetype filetype(
			fileloader,
			engine::Asset("tmpfiletype"),
			[](engine::file::loader & /*fileloader*/, core::content & content, utility::any & stash, engine::Asset file)
		{
			FileData & file_data = stash.emplace<FileData>();

			switch (file)
			{
			case engine::Hash(u8"maybe.exists"):
				file_data.value += int(read_char(content));
				break;
			case engine::Hash(u8"folder/maybe.exists"):
				file_data.value += int(read_char(content));
				break;
			default:
				debug_fail();
			}
		},
			[](engine::file::loader & /*fileloader*/, utility::any & stash, engine::Asset file)
		{
			if (!debug_assert(stash.type_id() == utility::type_id<FileData>()))
				return;

			FileData & file_data = utility::any_cast<FileData &>(stash);

			switch (file)
			{
			case engine::Hash(u8"maybe.exists"):
				file_data.value -= 3;
				break;
			case engine::Hash(u8"folder/maybe.exists"):
				file_data.value -= 4;
				break;
			default:
				debug_fail();
			}

			if (file_data.unload_value)
			{
				*file_data.unload_value = file_data.value;
			}
			if (file_data.unload_event)
			{
				file_data.unload_event->set();
			}
		});

		struct SyncData
		{
			int ready_value = {};
			int unload_value = {};
			core::sync::Event<true> ready_event;
			core::sync::Event<true> unload_event;
		} sync_data[2];

		engine::file::load_independent(
			fileloader,
			engine::Token(engine::Asset("my independent load")),
			engine::Asset(u8"maybe.exists"),
			filetype,
			[](engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset /*name*/, const utility::any & stash, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;
			if (!debug_assert(stash.type_id() == utility::type_id<FileData>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);
			const FileData & file_data = utility::any_cast<const FileData &>(stash);

			const_cast<FileData &>(file_data).unload_value = &sync_data->unload_value; // suspicious
			const_cast<FileData &>(file_data).unload_event = &sync_data->unload_event; // suspicious

			sync_data->ready_value = file_data.value;
			sync_data->ready_event.set();
		},
			[](engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset /*name*/, const utility::any & stash, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;
			if (!debug_assert(stash.type_id() == utility::type_id<FileData>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);
			const FileData & file_data = utility::any_cast<const FileData &>(stash);

			sync_data->ready_value -= file_data.value;
		},
			utility::any(sync_data + 0));

		engine::file::load_independent(
			fileloader,
			engine::Token(engine::Asset("my other independent load")),
			engine::Asset(u8"folder/maybe.exists"),
			filetype,
			[](engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset /*name*/, const utility::any & stash, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;
			if (!debug_assert(stash.type_id() == utility::type_id<FileData>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);
			const FileData & file_data = utility::any_cast<const FileData &>(stash);

			const_cast<FileData &>(file_data).unload_value = &sync_data->unload_value; // suspicious
			const_cast<FileData &>(file_data).unload_event = &sync_data->unload_event; // suspicious

			sync_data->ready_value = file_data.value;
			sync_data->ready_event.set();
		},
			[](engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset /*name*/, const utility::any & stash, engine::Asset /*file*/)
		{
			if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
				return;
			if (!debug_assert(stash.type_id() == utility::type_id<FileData>()))
				return;

			SyncData * const sync_data = utility::any_cast<SyncData *>(data);
			const FileData & file_data = utility::any_cast<const FileData &>(stash);

			sync_data->ready_value -= file_data.value;
		},
			utility::any(sync_data + 1));

		REQUIRE(sync_data[0].ready_event.wait(timeout));
		REQUIRE(sync_data[1].ready_event.wait(timeout));
		CHECK(sync_data[0].ready_value == 2);
		CHECK(sync_data[1].ready_value == 3);
		CHECK(sync_data[0].unload_value == 0);
		CHECK(sync_data[1].unload_value == 0);

		engine::file::unload_independent(fileloader, engine::Token(engine::Asset("my independent load")));
		engine::file::unload_independent(fileloader, engine::Token(engine::Asset("my other independent load")));

		REQUIRE(sync_data[0].unload_event.wait(timeout));
		REQUIRE(sync_data[1].unload_event.wait(timeout));
		CHECK(sync_data[0].ready_value == 0);
		CHECK(sync_data[1].ready_value == 0);
		CHECK(sync_data[0].unload_value == -1);
		CHECK(sync_data[1].unload_value == -1);
	}
}

namespace
{
	struct TreeFileData
	{
		int value = {};

		int * count = nullptr;
		int * unload_value = nullptr;
		core::sync::Event<true> * unload_event = nullptr;
	};

	struct SyncData
	{
		int ready_values[6] = {};
		int unload_values[6] = {};
		int count = {};
		core::sync::Event<true> ready_event;
		core::sync::Event<true> watch_event;
		core::sync::Event<true> unload_event;
	} sync_data;

	void tree_ready(engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset name, const utility::any & stash, engine::Asset /*file*/)
	{
		if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
			return;
		if (!debug_assert(stash.type_id() == utility::type_id<TreeFileData>()))
			return;

		SyncData * const sync_data_ = utility::any_cast<SyncData *>(data);
		const TreeFileData & file_data = utility::any_cast<const TreeFileData &>(stash);

		const_cast<TreeFileData &>(file_data).count = &sync_data_->count; // suspicious
		const_cast<TreeFileData &>(file_data).unload_event = &sync_data_->unload_event; // suspicious

		switch (name)
		{
		case engine::Hash(u8"tree.root"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 0; // suspicious
			sync_data_->ready_values[0]++;
			sync_data_->ready_event.set();
			break;
		case engine::Hash(u8"dependency.1"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 1; // suspicious
			sync_data_->ready_values[1]++;
			break;
		case engine::Hash(u8"dependency.2"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 2; // suspicious
			sync_data_->ready_values[2]++;
			sync_data_->watch_event.set();
			break;
		case engine::Hash(u8"dependency.3"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 3; // suspicious
			sync_data_->ready_values[3]++;
			break;
		case engine::Hash(u8"dependency.4"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 4; // suspicious
			sync_data_->ready_values[4]++;
			break;
		case engine::Hash(u8"dependency.5"):
			const_cast<TreeFileData &>(file_data).unload_value = sync_data_->unload_values + 5; // suspicious
			sync_data_->ready_values[5]++;
			break;
		default:
			debug_unreachable();
		}
	}

	void tree_unready(engine::file::loader & /*fileloader*/, utility::any & data, engine::Asset name, const utility::any & /*stash*/, engine::Asset /*file*/)
	{
		if (!debug_assert(data.type_id() == utility::type_id<SyncData *>()))
			return;

		SyncData * const sync_data_ = utility::any_cast<SyncData *>(data);

		switch (name)
		{
		case engine::Hash(u8"tree.root"):
			sync_data_->ready_values[0]--;
			break;
		case engine::Hash(u8"dependency.1"):
			sync_data_->ready_values[1]--;
			break;
		case engine::Hash(u8"dependency.2"):
			sync_data_->ready_values[2]--;
			break;
		case engine::Hash(u8"dependency.3"):
			sync_data_->ready_values[3]--;
			break;
		case engine::Hash(u8"dependency.4"):
			sync_data_->ready_values[4]--;
			break;
		case engine::Hash(u8"dependency.5"):
			sync_data_->ready_values[5]--;
			break;
		default:
			debug_unreachable();
		}
	}

	void tree_load(engine::file::loader & fileloader, core::content & content, utility::any & stash, engine::Asset file)
	{
		TreeFileData & file_data = stash.emplace<TreeFileData>();

		switch (file)
		{
		case engine::Hash(u8"tree.root"):
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.1"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.2"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.3"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			file_data.value += int(read_char(content));
			break;
		case engine::Hash(u8"dependency.1"):
			file_data.value += int(read_char(content));
			break;
		case engine::Hash(u8"dependency.2"):
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.3"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.4"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			file_data.value += int(read_char(content));
			break;
		case engine::Hash(u8"dependency.3"):
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.1"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			file_data.value += int(read_char(content));
			break;
		case engine::Hash(u8"dependency.4"):
			engine::file::load_dependency(fileloader, file, engine::Asset(u8"dependency.5"), engine::Asset("tmpfiletype"), tree_ready, tree_unready, utility::any(&sync_data));
			file_data.value += int(read_char(content));
			break;
		case engine::Hash(u8"dependency.5"):
			file_data.value += int(read_char(content));
			break;
		default:
			debug_fail();
		}
	}

	void tree_unload(engine::file::loader & /*fileloader*/, utility::any & stash, engine::Asset file)
	{
		if (!debug_assert(stash.type_id() == utility::type_id<TreeFileData>()))
			return;

		TreeFileData & file_data = utility::any_cast<TreeFileData &>(stash);

		switch (file)
		{
		case engine::Hash(u8"tree.root"):
			*file_data.unload_value -= 1;
			break;
		case engine::Hash(u8"dependency.1"):
			*file_data.unload_value -= 11;
			break;
		case engine::Hash(u8"dependency.2"):
			*file_data.unload_value -= 12;
			break;
		case engine::Hash(u8"dependency.3"):
			*file_data.unload_value -= 13;
			break;
		case engine::Hash(u8"dependency.4"):
			*file_data.unload_value -= 14;
			break;
		case engine::Hash(u8"dependency.5"):
			*file_data.unload_value -= 15;
			break;
		default:
			debug_fail();
		}

		(*file_data.count)++;
		if (*file_data.count == 6)
		{
			file_data.unload_event->set();
		}
	}
}

TEST_CASE("file loader can load tree", "[engine][file]")
{
	engine::task::scheduler taskscheduler(1);
	engine::file::system filesystem(taskscheduler, engine::file::directory::working_directory(), engine::file::config_t{});
	engine::file::loader fileloader(taskscheduler, filesystem);

	engine::file::scoped_directory tmpdir(filesystem, engine::Asset("tmpdir"));

	SECTION("")
	{
		ful::heap_string_utf8 filepath0;
		ful::assign(filepath0, ful::cstr_utf8("tree.root"));
		ful::heap_string_utf8 filepath1;
		ful::assign(filepath1, ful::cstr_utf8("dependency.1"));
		ful::heap_string_utf8 filepath2;
		ful::assign(filepath2, ful::cstr_utf8("dependency.2"));
		ful::heap_string_utf8 filepath3;
		ful::assign(filepath3, ful::cstr_utf8("dependency.3"));
		ful::heap_string_utf8 filepath4;
		ful::assign(filepath4, ful::cstr_utf8("dependency.4"));
		ful::heap_string_utf8 filepath5;
		ful::assign(filepath5, ful::cstr_utf8("dependency.5"));
		engine::file::write(filesystem, tmpdir, std::move(filepath0), engine::Asset{}, write_char, utility::any(char(1)));
		engine::file::write(filesystem, tmpdir, std::move(filepath1), engine::Asset{}, write_char, utility::any(char(11)));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Asset{}, write_char, utility::any(char(12)));
		engine::file::write(filesystem, tmpdir, std::move(filepath3), engine::Asset{}, write_char, utility::any(char(13)));
		engine::file::write(filesystem, tmpdir, std::move(filepath4), engine::Asset{}, write_char, utility::any(char(14)));
		engine::file::write(filesystem, tmpdir, std::move(filepath5), engine::Asset{}, write_char, utility::any(char(15)));

		engine::file::scoped_library tmplib(fileloader, tmpdir);

		engine::file::scoped_filetype filetype(fileloader, engine::Asset("tmpfiletype"), tree_load, tree_unload);

		engine::file::load_independent(fileloader, engine::Token(engine::Asset("tree root")), engine::Asset(u8"tree.root"), filetype, tree_ready, tree_unready, utility::any(&sync_data));

		REQUIRE(sync_data.ready_event.wait(timeout));
		CHECK(sync_data.ready_values[0] == 1);
		CHECK(sync_data.ready_values[1] == 2);
		CHECK(sync_data.ready_values[2] == 1);
		CHECK(sync_data.ready_values[3] == 2);
		CHECK(sync_data.ready_values[4] == 1);
		CHECK(sync_data.ready_values[5] == 1);

		sync_data.watch_event.reset();

		ful::assign(filepath2, ful::cstr_utf8("dependency.2"));
		engine::file::write(filesystem, tmpdir, std::move(filepath2), engine::Asset{}, write_char, utility::any(char(21)), engine::file::flags::OVERWRITE_EXISTING);

		REQUIRE(sync_data.watch_event.wait(timeout));
		CHECK(sync_data.ready_values[0] == 1);
		CHECK(sync_data.ready_values[1] == 2);
		CHECK(sync_data.ready_values[2] == 1);
		CHECK(sync_data.ready_values[3] == 2);
		CHECK(sync_data.ready_values[4] == 1);
		CHECK(sync_data.ready_values[5] == 1);

		engine::file::unload_independent(fileloader, engine::Token(engine::Asset("tree root")));

		REQUIRE(sync_data.unload_event.wait(timeout));
		CHECK(sync_data.ready_values[0] == 0);
		CHECK(sync_data.ready_values[1] == 0);
		CHECK(sync_data.ready_values[2] == 0);
		CHECK(sync_data.ready_values[3] == 0);
		CHECK(sync_data.ready_values[4] == 0);
		CHECK(sync_data.ready_values[5] == 0);
		CHECK(sync_data.unload_values[0] == -1);
		CHECK(sync_data.unload_values[1] == -11);
		CHECK(sync_data.unload_values[2] == -12);
		CHECK(sync_data.unload_values[3] == -13);
		CHECK(sync_data.unload_values[4] == -14);
		CHECK(sync_data.unload_values[5] == -15);
	}
}
