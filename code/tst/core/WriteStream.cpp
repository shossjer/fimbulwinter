#include "core/WriteStream.hpp"

#include <catch2/catch.hpp>

TEST_CASE("WriteStream can write", "[core][stream]")
{
	const char src[] = u8"abc1234";

	char dest[sizeof src] = {};

	struct dest_data
	{
		char * begin;
		char * end;
	} dest_data{dest, dest + sizeof src};

	core::WriteStream ws(
		[](const void * src, ext::usize n, void * data)
		{
			auto & dest_data = *static_cast<struct dest_data *>(data);

			// limit writes to at most three bytes,
			// this makes it possible to test both
			// `write_some` and `write_all`
			n = 3 < n ? 3 : n;

			for (ext::index i = 0; ext::usize(i) < n; i++)
			{
				dest_data.begin[i] = static_cast<const char *>(src)[i];
			}
			dest_data.begin += n;

			return ext::ssize(n);
		},
		&dest_data,
		ful::cstr_utf8(""));

	SECTION("some")
	{
		CHECK(ws.write_some(src, sizeof src) == 3);
		CHECK_FALSE(ws.done());
		CHECK_FALSE(ws.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == (i < 3 ? src[i] : 0));
		}
		CHECK(dest_data.begin + sizeof src - 3  == dest_data.end);
	}

	SECTION("all")
	{
		CHECK(ws.write_all(src, sizeof src) == sizeof src);
		CHECK_FALSE(ws.done());
		CHECK_FALSE(ws.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == src[i]);
		}
		CHECK(dest_data.begin == dest_data.end);
	}
}

TEST_CASE("WriteStream reports errors", "[core][stream]")
{
	core::WriteStream ws(
		[](const void * /*src*/, ext::usize /*n*/, void * /*data*/)
		{
			return ext::ssize(-1);
		},
		nullptr,
		ful::cstr_utf8(""));

	SECTION("writing some")
	{
		CHECK(ws.write_some(nullptr, 18) == 0);
		CHECK(ws.done());
		CHECK(ws.fail());
	}

	SECTION("writing all")
	{
		CHECK(ws.write_some(nullptr, 18) == 0);
		CHECK(ws.done());
		CHECK(ws.fail());
	}
}
