#include "utility/string.hpp"

#include <catch2/catch.hpp>

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

TEST_CASE("string can resize", "[utility][string]")
{
	utility::static_string<11, char> a(5);
	CHECK(a.size() == 5);

	SECTION("to something bigger")
	{
		CHECK(a.try_resize(8));
		CHECK(a.size() == 8);
	}

	SECTION("to something smaller")
	{
		CHECK(a.try_resize(2));
		CHECK(a.size() == 2);
	}

	SECTION("to the same")
	{
		CHECK(a.try_resize(5));
		CHECK(a.size() == 5);
	}
}
