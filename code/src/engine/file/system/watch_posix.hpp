#pragma once

#include "engine/Asset.hpp"
#include "engine/file/system/common.hpp"
#include "engine/Identity.hpp"

#include "utility/any.hpp"
#include "utility/container/vector.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/unicode/string.hpp"

namespace engine
{
	namespace file
	{
		struct system_impl;

		void initialize_watch();
		void deinitialize_watch();

		struct ReadData
		{
			engine::file::system_impl & impl;

			utility::heap_string_utf8 filepath;
			std::uint32_t root;

			engine::Asset strand;
			engine::file::read_callback * callback;
			utility::any data;
		};

		struct ScanData
		{
			engine::file::system_impl & impl;

			utility::heap_string_utf8 dirpath;
			engine::Asset directory;

			engine::Asset strand;
			engine::file::scan_callback * callback;
			utility::any data;

			utility::heap_string_utf8 files;
		};

		void add_file_watch(engine::Identity id, ext::heap_shared_ptr<ReadData> ptr, bool report_missing);
		void add_scan_watch(engine::Identity id, ext::heap_shared_ptr<ScanData> ptr, bool recurse_directories);
		void remove_watch(engine::Identity id);

		// todo ^^^ are in the watch while vvv are in the system :dizzy_face:

		struct FileMissingWork
		{
			ext::heap_shared_ptr<ReadData> ptr;
		};

		struct FileReadWork
		{
			ext::heap_shared_ptr<ReadData> ptr;
		};

		struct ScanChangeWork
		{
			ext::heap_shared_ptr<ScanData> ptr;
			utility::heap_string_utf8 filepath; // (sub)directory
			utility::heap_vector<utility::heap_string_utf8> files;
		};

		struct ScanOnceWork
		{
			ext::heap_shared_ptr<ScanData> ptr;
		};

		struct ScanRecursiveWork
		{
			ext::heap_shared_ptr<ScanData> ptr;
		};

		void post_work(FileMissingWork && data);
		void post_work(FileReadWork && data);
		void post_work(ScanChangeWork && data);
		void post_work(ScanOnceWork && data);
		void post_work(ScanRecursiveWork && data);
	}
}
