#include "config.h"

#if FILE_SYSTEM_USE_KERNEL32

#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/native/file.hpp"

#include "ful/convert.hpp"
#include "ful/heap.hpp"
#include "ful/string_init.hpp"

#include <Windows.h>

namespace core
{
	namespace native
	{
		int try_read_file(ful::cstr_utf8 filepath, bool (* callback)(core::content & content, void * data), void * data)
		{
			ful::heap_string_utfw wide_filepath; // todo static
			if (!convert(filepath.begin(), filepath.end(), wide_filepath))
				return 0;

			HANDLE hFile = ::CreateFileW(wide_filepath.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				debug_assert(::GetLastError() == ERROR_FILE_NOT_FOUND, "CreateFileW \"", filepath, "\" failed with last error ", ::GetLastError());
				return 0;
			}

			LARGE_INTEGER file_size;
			if (!debug_verify(::GetFileSizeEx(hFile, &file_size) != FALSE, "failed with last error ", ::GetLastError()))
			{
				debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

				return 0;
			}

			if (file_size.QuadPart != 0)
			{
				HANDLE hMappingObject = ::CreateFileMappingW(hFile, nullptr, PAGE_WRITECOPY, 0, 0, nullptr);
				if (!debug_verify(hMappingObject != (HANDLE)NULL, "CreateFileMappingW failed with last error ", ::GetLastError()))
				{
					debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

					return 0;
				}

				LPVOID file_view = ::MapViewOfFile(hMappingObject, FILE_MAP_COPY, 0, 0, 0);
				if (!debug_verify(file_view != (LPVOID)NULL, "MapViewOfFile failed with last error ", ::GetLastError()))
				{
					debug_verify(::CloseHandle(hMappingObject) != FALSE, "failed with last error ", ::GetLastError());
					debug_verify(::CloseHandle(hFile) != FALSE, "failed with last error ", ::GetLastError());

					return 0;
				}

				core::content content(filepath, file_view, file_size.QuadPart);

				const bool ret = callback(content, data);

				debug_verify(::UnmapViewOfFile(file_view) != FALSE, "failed with last error ", ::GetLastError());
				debug_verify(::CloseHandle(hMappingObject) != FALSE, "failed with last error ", ::GetLastError());
				debug_verify(::CloseHandle(hFile) != FALSE, " failed with last error ", ::GetLastError());

				return ret ? 1 : -1;
			}
			else
			{
				core::content content(filepath, nullptr, file_size.QuadPart);

				const bool ret = callback(content, data);

				debug_verify(::CloseHandle(hFile) != FALSE, " failed with last error ", ::GetLastError());

				return ret ? 1 : -1;
			}
		}
	}
}

#endif
