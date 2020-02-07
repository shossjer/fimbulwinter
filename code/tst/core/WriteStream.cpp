#include "core/WriteStream.hpp"

#include <catch/catch.hpp>

TEST_CASE("WriteStream calls its callback", "[core][stream]")
{
	const char src[] = u8"abc123";

	char dest[sizeof src] = {};

	core::WriteStream ws([](const char * src, int64_t n, void * data)
	                     {
		                     char * const dest = static_cast<char *>(data);

		                     for (ext::index i = 0; i < n; i++)
		                     {
			                     dest[i] = src[i];
		                     }

		                     return uint64_t(n);
	                     },
	                     dest,
	                     u8"");

	CHECK(ws.write(src, sizeof src) == sizeof src);
	CHECK(ws.valid());
	for (ext::index i = 0; i < ext::ssize(sizeof src); i++)
	{
		CHECK(dest[i] == src[i]);
	}
}

TEST_CASE("WriteStream reports errors", "[core][stream]")
{
	core::WriteStream ws([](const char * /*src*/, int64_t n, void * /*data*/)
	                     {
		                     CHECK(0 <= n);

		                     const auto half = n / 2;
		                     return 0x8000000000000000 | half;
	                     },
	                     nullptr,
	                     u8"");

	CHECK(ws.write(nullptr, 18) == 9);
	CHECK_FALSE(ws.valid());
}
