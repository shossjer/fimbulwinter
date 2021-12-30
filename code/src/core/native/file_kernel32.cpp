#include "config.h"

#if FILE_SYSTEM_USE_KERNEL32

#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/native/file.hpp"

#include "ful/convert.hpp"
#include "ful/heap.hpp"
#include "ful/string_init.hpp"

#include <utility>

#include <Windows.h>

namespace core
{
	namespace native
	{
		bool try_read_file(ful::cstr_utf8 filepath, void (* callback)(core::content & content, void * data), void * data)
		{
			ful::heap_string_utfw wide_filepath; // todo static
			if (!convert(filepath.begin(), filepath.end(), wide_filepath))
				return false;

			HANDLE hFile = ::CreateFileW(wide_filepath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				debug_assert(::GetLastError() == ERROR_FILE_NOT_FOUND, "CreateFileW \"", filepath, "\" failed with last error ", ::GetLastError());
				return false;
			}

			LARGE_INTEGER file_size;
			if (!debug_verify(::GetFileSizeEx(hFile, &file_size) != FALSE, "failed with last error ", ::GetLastError()))
			{
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return false;
			}

			HANDLE hMappingObject = ::CreateFileMappingW(hFile, nullptr, PAGE_WRITECOPY, 0, 0, nullptr);
			if (!debug_verify(hMappingObject != (HANDLE)NULL, "CreateFileMappingW failed with last error ", ::GetLastError()))
			{
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return false;
			}

			LPVOID file_view = ::MapViewOfFile(hMappingObject, FILE_MAP_COPY, 0, 0, 0);
			if (!debug_verify(file_view != (LPVOID)NULL, "MapViewOfFile failed with last error ", ::GetLastError()))
			{
				debug_verify(::CloseHandle(hMappingObject) != FALSE, "failed with last error ", ::GetLastError());
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return false;
			}

			core::content content(filepath, file_view, file_size.QuadPart);

			callback(content, data);

			debug_verify(::UnmapViewOfFile(file_view) != FALSE, "failed with last error ", ::GetLastError());
			debug_verify(::CloseHandle(hMappingObject) != FALSE, "failed with last error ", ::GetLastError());
			debug_verify(::CloseHandle(hFile) != FALSE, " failed with last error ", ::GetLastError());

			return true;
		}
	}
}

#endif
