#pragma once

#include "engine/Asset.hpp"
#include "engine/file/system/callbacks.hpp"
#include "engine/Identity.hpp"

#include "utility/any.hpp"
#include "utility/container/vector.hpp"
#include "utility/shared_ptr.hpp"
#include "utility/unicode/string.hpp"

#if FILE_SYSTEM_USE_KERNEL32
# include <Windows.h>
#endif

namespace engine
{
	namespace file
	{
		struct system_impl;

		struct ReadData
		{
			engine::file::system_impl & impl;

#if FILE_SYSTEM_USE_KERNEL32
			utility::heap_string_utfw filepath;
#elif FILE_SYSTEM_USE_POSIX
			utility::heap_string_utf8 filepath;
#endif
			std::uint32_t root;

			engine::Asset strand;
			engine::file::read_callback * callback;
			utility::any data;

#if FILE_SYSTEM_USE_KERNEL32
			FILETIME last_write_time;
#endif
		};

		struct ScanData
		{
			engine::file::system_impl & impl;

#if FILE_SYSTEM_USE_KERNEL32
			utility::heap_string_utfw dirpath;
#elif FILE_SYSTEM_USE_POSIX
			utility::heap_string_utf8 dirpath;
#endif
			engine::Asset directory;

			engine::Asset strand;
			engine::file::scan_callback * callback;
			utility::any data;

#if FILE_SYSTEM_USE_KERNEL32
			utility::heap_string_utfw files;
#elif FILE_SYSTEM_USE_POSIX
			utility::heap_string_utf8 files;
#endif
		};

		struct WriteData
		{
			engine::file::system_impl & impl;

#if FILE_SYSTEM_USE_KERNEL32
			utility::heap_string_utfw filepath;
#elif FILE_SYSTEM_USE_POSIX
			utility::heap_string_utf8 filepath;
#endif
			std::uint32_t root;

			engine::Asset strand;
			engine::file::write_callback * callback;
			utility::any data;

			bool append : 1;
			bool overwrite : 1;
		};

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
#if FILE_SYSTEM_USE_KERNEL32
			utility::heap_string_utfw filepath; // (sub)directory
			utility::heap_vector<utility::heap_string_utfw> files;
#elif FILE_SYSTEM_USE_POSIX
			utility::heap_string_utf8 filepath; // (sub)directory
			utility::heap_vector<utility::heap_string_utf8> files;
#endif
		};

		struct ScanOnceWork
		{
			ext::heap_shared_ptr<ScanData> ptr;
		};

		struct ScanRecursiveWork
		{
			ext::heap_shared_ptr<ScanData> ptr;
		};

		struct FileWriteWork
		{
			ext::heap_shared_ptr<WriteData> ptr;
		};

		void post_work(FileMissingWork && data);
		void post_work(FileReadWork && data);
		void post_work(ScanChangeWork && data);
		void post_work(ScanOnceWork && data);
		void post_work(ScanRecursiveWork && data);
		void post_work(FileWriteWork && data);
	}
}
