#include "config.h"

#if FILE_SYSTEM_USE_KERNEL32

#include "file.hpp"

#include "core/ReadStream.hpp"

#include <utility>

#include <Windows.h>

namespace core
{
	namespace native
	{
		bool try_read_file(utility::heap_string_utf8 && filepath, void(* callback)(core::ReadStream && stream, void * data), void * data)
		{
			HANDLE hFile = ::CreateFileW(utility::heap_widen(filepath).data(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				debug_assert(::GetLastError() == ERROR_FILE_NOT_FOUND, "CreateFileW \"", filepath, "\" failed with last error ", ::GetLastError());
				return false;
			}

			callback(
				core::ReadStream(
					[](void * dest, ext::usize n, void * data)
			{
				HANDLE hFile = reinterpret_cast<HANDLE>(data);

				DWORD read;
				if (!debug_verify(::ReadFile(hFile, dest, debug_cast<DWORD>(n), &read, nullptr) != FALSE, " failed with last error ", ::GetLastError()))
					return ext::ssize(-1);

				return ext::ssize(read);
			},
					reinterpret_cast<void *>(hFile),
					std::move(filepath)),
				data);

			debug_verify(::CloseHandle(hFile) != FALSE, " failed with last error ", ::GetLastError());
			return true;
		}
	}
}

#endif
