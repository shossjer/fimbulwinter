#pragma once

#include "engine/Identity.hpp"
#include "engine/file/system/works.hpp"

#include "utility/shared_ptr.hpp"

namespace engine
{
	namespace file
	{
		void initialize_watch();
		void deinitialize_watch();

		void add_file_watch(engine::Identity id, ext::heap_shared_ptr<ReadData> ptr, bool report_missing);
		void add_scan_watch(engine::Identity id, ext::heap_shared_ptr<ScanData> ptr, bool recurse_directories);
		void remove_watch(engine::Identity id);
	}
}
