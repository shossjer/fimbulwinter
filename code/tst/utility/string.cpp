#include "utility/string.hpp"

#include <catch/catch.hpp>

TEST_CASE("string view can find", "[utility][string view]")
{
	utility::basic_string_view<char> a = "abcddf";

	SECTION("existing characters")
	{
		CHECK(a.find('a') == 0);
		CHECK(a.find('c') == 2);
		CHECK(a.find('f') == 5);
	}

	SECTION("existing characters in reverse")
	{
		CHECK(a.rfind('a') == 0);
		CHECK(a.rfind('c') == 2);
		CHECK(a.rfind('f') == 5);
	}

	SECTION("the first of multiple matches")
	{
		CHECK(a.find('d') == 3);
	}

	SECTION("the first of multiple matches in reverse")
	{
		CHECK(a.rfind('d') == 4);
	}
}

TEST_CASE("string view cannot find", "[utility][string view]")
{
	utility::basic_string_view<char> a = "abcdef";

	SECTION("nonexisting characters")
	{
		CHECK(a.find('g') == 6);
	}

	SECTION("nonexisting characters in reverse")
	{
		CHECK(a.rfind('g') == 6);
	}
}
