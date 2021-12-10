#include "core/StringStream.hpp"

#include <catch2/catch.hpp>

TEST_CASE("StringStream", "[core][stream]")
{
	const char src[] = u8"abc123def456ghi789jkl";

	struct src_data
	{
		const char * begin;
		const char * end;
	} src_data{src, src + sizeof src - 1};

	core::StringStream<utility::heap_storage_traits> stream(
		core::ReadStream(
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
			ful::cstr_utf8("")));

	SECTION("")
	{
		auto it = stream.begin();
		REQUIRE(it != stream.end());
		CHECK(*it == src[0]);

		++it;
		REQUIRE(it != stream.end());
		CHECK(*it == src[1]);

		++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it;
		REQUIRE(it != stream.end());
		CHECK(*it == src[9]);

		--it;
		REQUIRE(it != stream.end());
		CHECK(*it == src[8]);

		++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it; ++it;
		CHECK(it == stream.end());
	}
}
