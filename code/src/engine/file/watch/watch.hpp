#pragma once

#include "config.h"

#include "engine/Identity.hpp"
#include "engine/file/system/works.hpp"

#include "utility/shared_ptr.hpp"

namespace engine
{
	namespace file
	{
		struct watch_impl
		{
#if FILE_WATCH_USE_INOTIFY
			int fd;
#endif

			~watch_impl();

			watch_impl();
		};

#if FILE_WATCH_USE_INOTIFY
		void process_watch(watch_impl & impl);
#endif

		void add_file_watch(watch_impl & impl, engine::Identity id, ext::heap_shared_ptr<ReadData> ptr, bool report_missing);
		void add_scan_watch(watch_impl & impl, engine::Identity id, ext::heap_shared_ptr<ScanData> ptr, bool recurse_directories);
		void remove_watch(watch_impl & impl, engine::Identity id);
	}
}
