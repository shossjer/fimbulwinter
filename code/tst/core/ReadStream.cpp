#include "core/ReadStream.hpp"

#include <catch2/catch.hpp>

TEST_CASE("ReadStream", "[core][stream]")
{
	const char src[] = u8"abc123";

	char dest[sizeof src] = {};

	struct src_data
	{
		const char * begin;
		const char * end;
	} src_data{src, src + sizeof src};

	core::ReadStream rs(
		[](void * dest, ext::usize n, void * data)
		{
			auto & src_data = *static_cast<struct src_data *>(data);

			// limit reads to at most three bytes, this makes it
			// possible to test both `read_some` and `read_all`
			n = std::min({n, ext::usize(src_data.end - src_data.begin), ext::usize(3)});

			std::copy_n(src_data.begin, n, static_cast<char *>(dest));
			src_data.begin += n;

			return ext::ssize(n);
		},
		&src_data,
		ful::cstr_utf8(""));

	SECTION("can read some")
	{
		CHECK(rs.read_some(dest, sizeof dest) == 3);
		CHECK_FALSE(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == (i < 3 ? src[i] : 0));
		}
		CHECK(src_data.begin + sizeof src - 3  == src_data.end);

		CHECK(rs.read_some(dest + 3, 2) == 2);
		CHECK_FALSE(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == (i < 5 ? src[i] : 0));
		}
		CHECK(src_data.begin + sizeof src - 5 == src_data.end);

		CHECK(rs.read_some(dest + 5, sizeof dest - 5) == 2);
		CHECK_FALSE(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == src[i]);
		}
		CHECK(src_data.begin == src_data.end);

		CHECK(rs.read_some(dest, 1) == 0);
		CHECK(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == src[i]);
		}
		CHECK(src_data.begin == src_data.end);
	}

	SECTION("can read all")
	{
		CHECK(rs.read_all(dest, sizeof dest) == sizeof dest);
		CHECK_FALSE(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == src[i]);
		}
		CHECK(src_data.begin == src_data.end);

		CHECK(rs.read_all(dest, 1) == 0);
		CHECK(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == src[i]);
		}
		CHECK(src_data.begin == src_data.end);
	}

	SECTION("can skip")
	{
		CHECK(rs.skip(4) == 4);
		CHECK_FALSE(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == 0);
		}
		CHECK(src_data.begin + sizeof src - 4 == src_data.end);

		CHECK(rs.skip(4) == 3);
		CHECK(rs.done());
		CHECK_FALSE(rs.fail());
		for (ext::index i = 0; ext::usize(i) < sizeof src; i++)
		{
			CHECK(dest[i] == 0);
		}
		CHECK(src_data.begin == src_data.end);
	}
}

TEST_CASE("ReadStream reports errors", "[core][stream]")
{
	core::ReadStream rs([](void * /*dest*/, ext::usize /*n*/, void * /*data*/)
	                    {
		                    return ext::ssize(-1);
	                    },
	                    nullptr,
	                    ful::cstr_utf8(""));

	SECTION("writing some")
	{
		CHECK(rs.read_some(nullptr, 1) == 0);
		CHECK(rs.done());
		CHECK(rs.fail());
	}

	SECTION("writing all")
	{
		CHECK(rs.read_some(nullptr, 1) == 0);
		CHECK(rs.done());
		CHECK(rs.fail());
	}

	SECTION("skipping")
	{
		CHECK(rs.skip(1) == 0);
		CHECK(rs.done());
		CHECK(rs.fail());
	}
}
