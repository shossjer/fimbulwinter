#include "config.h"

#if FILE_SYSTEM_USE_POSIX

#include "file.hpp"

#include "core/ReadStream.hpp"

#include "utility/ext/unistd.hpp"

#include <utility>

#include <fcntl.h>

namespace core
{
	namespace native
	{
		bool try_read_file(utility::heap_string_utf8 && filepath, void (* callback)(core::ReadStream && stream, void * data), void * data)
		{
			const int fd = ::open(filepath.data(), O_RDONLY);
			if (fd == -1)
			{
				debug_assert(errno == ENOENT, "open \"", filepath, "\" failed with errno ", errno);
				return false;
			}

			callback(
				core::ReadStream(
					[](void * dest, ext::usize n, void * data)
					{
						const int fd = static_cast<int>(reinterpret_cast<std::intptr_t>(data));

						return ext::read_some_nonzero(fd, dest, n);
					},
					reinterpret_cast<void *>(fd),
					std::move(filepath)),
				data);

			debug_verify(::close(fd) != -1, "failed with errno ", errno);
			return true;
		}
	}
}

#endif
