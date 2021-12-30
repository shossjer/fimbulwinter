#include "config.h"

#if FILE_SYSTEM_USE_POSIX

#include "core/content.hpp"
#include "core/debug.hpp"
#include "core/native/file.hpp"

#include <utility>

#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace core
{
	namespace native
	{
		bool try_read_file(ful::cstr_utf8 filepath, void (* callback)(core::content & content, void * data), void * data)
		{
			const int fd = ::open(filepath.c_str(), O_RDONLY);
			if (fd == -1)
			{
				debug_assert(errno == ENOENT, "open \"", filepath, "\" failed with errno ", errno);
				return false;
			}

			struct stat statbuf;
			debug_verify(::fstat(fd, &statbuf) == 0, "failed with errno ", errno);

			void * map = ::mmap(nullptr, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
			if (map == MAP_FAILED)
			{
				debug_verify(::close(fd) == 0, "failed with errno ", errno);

				return false;
			}

			core::content content(filepath, map, statbuf.st_size);

			callback(content, data);

			debug_verify(::munmap(map, statbuf.st_size) == 0, "failed with errno ", errno);
			debug_verify(::close(fd) == 0, "failed with errno ", errno);

			return true;
		}
	}
}

#endif
