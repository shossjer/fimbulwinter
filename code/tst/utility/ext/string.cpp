#include "utility/ext/string.hpp"

#include <catch/catch.hpp>

TEST_CASE("strlen", "[utility][cstring][ext]")
{
	SECTION("stops at null terminator")
	{
		CHECK(ext::strlen("") == 0);
		CHECK(ext::strlen("123") == 3);
	}

	SECTION("stops at arbitrary delimiter")
	{
		CHECK(ext::strlen("a", 'a') == 0);
		CHECK(ext::strlen("bbbabb", 'a') == 3);
	}
}
