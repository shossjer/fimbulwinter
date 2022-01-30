#pragma once

#include "utility/ext/stddef.hpp"

#include <cassert>
#include <unistd.h>

namespace ext
{

	// fd - blocking file descriptor
	inline ssize write_some_nonzero(int fd, const void * buffer, usize size)
	{
		assert(size != 0);

	try_again:
		const ssize n = ::write(fd, buffer, size);
		if (n < 0)
		{
			if (errno == EINTR)
				goto try_again;
		}
		return n;
	}

	// fd - blocking file descriptor
	inline ssize write_some(int fd, const void * buffer, usize size)
	{
		return size == 0 ? 0 : write_some_nonzero(fd, buffer, size);
	}

	// fd - blocking file descriptor
	inline usize write_all_nonzero(int fd, const void * buffer, usize size)
	{
		const char * ptr = static_cast<const char *>(buffer);
		usize remaining = size;

		do
		{
			assert(remaining <= size);

			const ssize n = write_some_nonzero(fd, ptr, remaining);
			if (n <= 0)
				break;

			assert(static_cast<usize>(n) <= remaining);
			ptr += n;
			remaining -= n;
		}
		while (0 < remaining);

		return size - remaining;
	}

	// fd - blocking file descriptor
	inline usize write_all(int fd, const void * buffer, usize size)
	{
		return size == 0 ? 0 : write_all_nonzero(fd, buffer, size);
	}
}
