#include "utility/string.hpp"

#include <catch2/catch.hpp>

TEST_CASE("string view", "[utility][string view]")
{
	utility::basic_string_view<utility::boundary_unit<char>> a = "abcddf";

	SECTION("can find existing characters")
	{
		CHECK(find(a, 'a') == a.begin() + 0);
		CHECK(find(a, 'c') == a.begin() + 2);
		CHECK(find(a, 'f') == a.begin() + 5);
	}

	SECTION("can find existing characters in reverse")
	{
		CHECK(rfind(a, 'a') == a.begin() + 0);
		CHECK(rfind(a, 'c') == a.begin() + 2);
		CHECK(rfind(a, 'f') == a.begin() + 5);
	}

	SECTION("can find the first of multiple matches")
	{
		CHECK(find(a, 'd') == a.begin() + 3);
	}

	SECTION("can find the first of multiple matches in reverse")
	{
		CHECK(rfind(a, 'd') == a.begin() + 4);
	}

	SECTION("cannot find nonexisting characters")
	{
		CHECK(find(a, 'g') == a.begin() + 6);
	}

	SECTION("cannot find nonexisting characters in reverse")
	{
		CHECK(rfind(a, 'g') == a.begin() + 6);
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

TEST_CASE("trivial static string", "[utility][string]")
{
	utility::static_string<11, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 0);

	CHECK(a.try_push_back('a'));
	CHECK(a.capacity() == 10);
	REQUIRE(a.size() == 1);
	CHECK(a.data()[0] == 'a');

	SECTION("can be copy constructed")
	{
		utility::static_string<11, char> b = a;
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be copy assigned")
	{
		utility::static_string<11, char> b;
		b = a;
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be move constructed")
	{
		utility::static_string<11, char> b = std::move(a);
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be move assigned")
	{
		utility::static_string<11, char> b;
		b = std::move(a);
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}
}

TEST_CASE("trivial heap string", "[utility][string]")
{
	utility::heap_string<char> a;
	CHECK(a.capacity() >= 0);
	CHECK(a.size() == 0);

	CHECK(a.try_push_back('a'));
	CHECK(a.capacity() >= 1);
	REQUIRE(a.size() == 1);
	CHECK(a.data()[0] == 'a');

	SECTION("can be copy constructed")
	{
		utility::heap_string<char> b = a;
		CHECK(a.capacity() >= 1);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be copy assigned")
	{
		utility::heap_string<char> b;
		b = a;
		CHECK(a.capacity() >= 1);
		REQUIRE(a.size() == 1);
		CHECK(a.data()[0] == 'a');
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be move constructed")
	{
		utility::heap_string<char> b = std::move(a);
		CHECK(a.capacity() >= 0);
		CHECK(a.size() == 0);
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}

	SECTION("can be move assigned")
	{
		utility::heap_string<char> b;
		b = std::move(a);
		CHECK(a.capacity() >= 0);
		CHECK(a.size() == 0);
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(b.data()[0] == 'a');
	}
}
