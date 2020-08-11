#include "utility/ext/string.hpp"

#include <catch2/catch.hpp>

TEST_CASE("strend", "[ext][string][utility]")
{
	SECTION("stops at null terminator")
	{
		CHECK(ext::strend("") == "");
		CHECK(ext::strend("123") == "123" + 3);
	}

	SECTION("stops at arbitrary delimiter")
	{
		CHECK(ext::strend("a", 'a') == "a");
		CHECK(ext::strend("bbbabb", 'a') == "bbbabb" + 3);
	}
}

TEST_CASE("strlen", "[ext][string][utility]")
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

TEST_CASE("strmatch", "[ext][string][utility]")
{
	SECTION("checks if range matches characters from the string in sequence")
	{
		CHECK(ext::strmatch("ab", "ab" + 0, "aa"));
		CHECK(ext::strmatch("ab", "ab" + 1, "aa"));
		CHECK_FALSE(ext::strmatch("ab", "ab" + 2, "aa"));

		CHECK(ext::strmatch("ab", "ab" + 0, "bb"));
		CHECK_FALSE(ext::strmatch("ab", "ab" + 1, "bb"));
		CHECK_FALSE(ext::strmatch("ab", "ab" + 2, "bb"));
	}
}

TEST_CASE("strbegins", "[ext][string][utility]")
{
	SECTION("checks if range begins with character")
	{
		CHECK_FALSE(ext::strbegins("ab", "ab" + 0, 'a'));
		CHECK(ext::strbegins("ab", "ab" + 1, 'a'));
		CHECK(ext::strbegins("ab", "ab" + 2, 'a'));

		CHECK_FALSE(ext::strbegins("ab", "ab" + 0, 'b'));
		CHECK_FALSE(ext::strbegins("ab", "ab" + 1, 'b'));
		CHECK_FALSE(ext::strbegins("ab", "ab" + 2, 'b'));
	}

	SECTION("checks if range begins with string")
	{
		CHECK(ext::strbegins("a", "a" + 0, ""));
		CHECK(ext::strbegins("a", "a" + 1, ""));

		CHECK_FALSE(ext::strbegins("abc", "abc" + 0, "ab"));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 1, "ab"));
		CHECK(ext::strbegins("abc", "abc" + 2, "ab"));
		CHECK(ext::strbegins("abc", "abc" + 3, "ab"));

		CHECK_FALSE(ext::strbegins("abc", "abc" + 0, "abb"));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 1, "abb"));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 2, "abb"));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 3, "abb"));
	}

	SECTION("checks if range begins like another range")
	{
		CHECK(ext::strbegins("a", "a" + 0, "b", "b" + 0));
		CHECK(ext::strbegins("a", "a" + 1, "b", "b" + 0));
		CHECK_FALSE(ext::strbegins("a", "a" + 0, "a", "a" + 1));
		CHECK(ext::strbegins("a", "a" + 1, "a", "a" + 1));

		CHECK_FALSE(ext::strbegins("abc", "abc" + 0, "abb", "abb" + 2));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 1, "abb", "abb" + 2));
		CHECK(ext::strbegins("abc", "abc" + 2, "abb", "abb" + 2));
		CHECK(ext::strbegins("abc", "abc" + 3, "abb", "abb" + 2));

		CHECK_FALSE(ext::strbegins("abc", "abc" + 0, "abb", "abb" + 3));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 1, "abb", "abb" + 3));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 2, "abb", "abb" + 3));
		CHECK_FALSE(ext::strbegins("abc", "abc" + 3, "abb", "abb" + 3));
	}
}

TEST_CASE("strncmp", "[ext][string][utility]")
{
	SECTION("")
	{
		CHECK(ext::strncmp("abc", "", 0) == 0);

		CHECK(ext::strncmp("abc", "a", 1) == 0);
		CHECK(ext::strncmp("abc", "z", 1) < 0);
		CHECK(ext::strncmp("abc", "0", 1) > 0);

		CHECK(ext::strncmp("abc", "abb", 0) == 0);
		CHECK(ext::strncmp("abc", "abb", 1) == 0);
		CHECK(ext::strncmp("abc", "abb", 2) == 0);
		CHECK(ext::strncmp("abc", "abb", 3) > 0);
	}
}

TEST_CASE("strcmp", "[ext][string][utility]")
{
	SECTION("checks how range compares to character")
	{
		CHECK(ext::strcmp("ab", "ab" + 0, 'a') < 0);
		CHECK(ext::strcmp("ab", "ab" + 1, 'a') == 0);
		CHECK(ext::strcmp("ab", "ab" + 2, 'a') > 0);

		CHECK(ext::strcmp("ab", "ab" + 0, 'b') < 0);
		CHECK(ext::strcmp("ab", "ab" + 1, 'b') < 0);
		CHECK(ext::strcmp("ab", "ab" + 2, 'b') < 0);
	}

	SECTION("checks how range compares to string")
	{
		CHECK(ext::strcmp("a", "a" + 0, "") == 0);
		CHECK(ext::strcmp("a", "a" + 1, "") > 0);

		CHECK(ext::strcmp("abc", "abc" + 0, "ab") < 0);
		CHECK(ext::strcmp("abc", "abc" + 1, "ab") < 0);
		CHECK(ext::strcmp("abc", "abc" + 2, "ab") == 0);
		CHECK(ext::strcmp("abc", "abc" + 3, "ab") > 0);

		CHECK(ext::strcmp("abc", "abc" + 0, "abb") < 0);
		CHECK(ext::strcmp("abc", "abc" + 1, "abb") < 0);
		CHECK(ext::strcmp("abc", "abc" + 2, "abb") < 0);
		CHECK(ext::strcmp("abc", "abc" + 3, "abb") > 0);
	}

	SECTION("checks how range compares to another range")
	{
		CHECK(ext::strcmp("a", "a" + 0, "b", "b" + 0) == 0);
		CHECK(ext::strcmp("a", "a" + 1, "b", "b" + 0) > 0);
		CHECK(ext::strcmp("a", "a" + 0, "a", "a" + 1) < 0);
		CHECK(ext::strcmp("a", "a" + 1, "a", "a" + 1) == 0);

		CHECK(ext::strcmp("abc", "abc" + 0, "abb", "abb" + 2) < 0);
		CHECK(ext::strcmp("abc", "abc" + 1, "abb", "abb" + 2) < 0);
		CHECK(ext::strcmp("abc", "abc" + 2, "abb", "abb" + 2) == 0);
		CHECK(ext::strcmp("abc", "abc" + 3, "abb", "abb" + 2) > 0);

		CHECK(ext::strcmp("abc", "abc" + 0, "abb", "abb" + 3) < 0);
		CHECK(ext::strcmp("abc", "abc" + 1, "abb", "abb" + 3) < 0);
		CHECK(ext::strcmp("abc", "abc" + 2, "abb", "abb" + 3) < 0);
		CHECK(ext::strcmp("abc", "abc" + 3, "abb", "abb" + 3) > 0);
	}
}

TEST_CASE("strfind", "[ext][string][utility]")
{
	SECTION("finds first occurrence of character")
	{
		CHECK(ext::strfind("aba", "aba" + 0, 'a') == "aba" + 0);
		CHECK(ext::strfind("aba", "aba" + 1, 'a') == "aba" + 0);
		CHECK(ext::strfind("aba", "aba" + 2, 'a') == "aba" + 0);
		CHECK(ext::strfind("aba", "aba" + 3, 'a') == "aba" + 0);

		CHECK(ext::strfind("abb", "abb" + 0, 'b') == "abb" + 0);
		CHECK(ext::strfind("abb", "abb" + 1, 'b') == "abb" + 1);
		CHECK(ext::strfind("abb", "abb" + 2, 'b') == "abb" + 1);
		CHECK(ext::strfind("abb", "abb" + 3, 'b') == "abb" + 1);
	}

	SECTION("finds first occurrence of another range")
	{
		CHECK(ext::strfind("aa", "aa" + 0, "a", "a" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 1, "a", "a" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 2, "a", "a" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 0, "a", "a" + 1) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 1, "a", "a" + 1) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 2, "a", "a" + 1) == "aa" + 0);

		CHECK(ext::strfind("aa", "aa" + 0, "b", "b" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 1, "b", "b" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 2, "b", "b" + 0) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 0, "b", "b" + 1) == "aa" + 0);
		CHECK(ext::strfind("aa", "aa" + 1, "b", "b" + 1) == "aa" + 1);
		CHECK(ext::strfind("aa", "aa" + 2, "b", "b" + 1) == "aa" + 2);

		CHECK(ext::strfind("abcbc", "abcbc" + 0, "bcd", "bcd" + 2) == "abcbc" + 0);
		CHECK(ext::strfind("abcbc", "abcbc" + 1, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strfind("abcbc", "abcbc" + 2, "bcd", "bcd" + 2) == "abcbc" + 2);
		CHECK(ext::strfind("abcbc", "abcbc" + 3, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strfind("abcbc", "abcbc" + 4, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strfind("abcbc", "abcbc" + 5, "bcd", "bcd" + 2) == "abcbc" + 1);

		CHECK(ext::strfind("abcbc", "abcbc" + 0, "bcd", "bcd" + 3) == "abcbc" + 0);
		CHECK(ext::strfind("abcbc", "abcbc" + 1, "bcd", "bcd" + 3) == "abcbc" + 1);
		CHECK(ext::strfind("abcbc", "abcbc" + 2, "bcd", "bcd" + 3) == "abcbc" + 2);
		CHECK(ext::strfind("abcbc", "abcbc" + 3, "bcd", "bcd" + 3) == "abcbc" + 3);
		CHECK(ext::strfind("abcbc", "abcbc" + 4, "bcd", "bcd" + 3) == "abcbc" + 4);
		CHECK(ext::strfind("abcbc", "abcbc" + 5, "bcd", "bcd" + 3) == "abcbc" + 5);
	}
}

TEST_CASE("strrfind", "[ext][string][utility]")
{
	SECTION("finds last occurrence of character")
	{
		CHECK(ext::strrfind("aba", "aba" + 0, 'a') == "aba" + 0);
		CHECK(ext::strrfind("aba", "aba" + 1, 'a') == "aba" + 0);
		CHECK(ext::strrfind("aba", "aba" + 2, 'a') == "aba" + 0);
		CHECK(ext::strrfind("aba", "aba" + 3, 'a') == "aba" + 2);

		CHECK(ext::strrfind("abb", "abb" + 0, 'b') == "abb" + 0);
		CHECK(ext::strrfind("abb", "abb" + 1, 'b') == "abb" + 1);
		CHECK(ext::strrfind("abb", "abb" + 2, 'b') == "abb" + 1);
		CHECK(ext::strrfind("abb", "abb" + 3, 'b') == "abb" + 2);
	}

	SECTION("finds last occurrence of another range")
	{
		CHECK(ext::strrfind("aa", "aa" + 0, "a", "a" + 0) == "aa" + 0);
		CHECK(ext::strrfind("aa", "aa" + 1, "a", "a" + 0) == "aa" + 1);
		CHECK(ext::strrfind("aa", "aa" + 2, "a", "a" + 0) == "aa" + 2);
		CHECK(ext::strrfind("aa", "aa" + 0, "a", "a" + 1) == "aa" + 0);
		CHECK(ext::strrfind("aa", "aa" + 1, "a", "a" + 1) == "aa" + 0);
		CHECK(ext::strrfind("aa", "aa" + 2, "a", "a" + 1) == "aa" + 1);

		CHECK(ext::strrfind("aa", "aa" + 0, "b", "b" + 0) == "aa" + 0);
		CHECK(ext::strrfind("aa", "aa" + 1, "b", "b" + 0) == "aa" + 1);
		CHECK(ext::strrfind("aa", "aa" + 2, "b", "b" + 0) == "aa" + 2);
		CHECK(ext::strrfind("aa", "aa" + 0, "b", "b" + 1) == "aa" + 0);
		CHECK(ext::strrfind("aa", "aa" + 1, "b", "b" + 1) == "aa" + 1);
		CHECK(ext::strrfind("aa", "aa" + 2, "b", "b" + 1) == "aa" + 2);

		CHECK(ext::strrfind("abcbc", "abcbc" + 0, "bcd", "bcd" + 2) == "abcbc" + 0);
		CHECK(ext::strrfind("abcbc", "abcbc" + 1, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strrfind("abcbc", "abcbc" + 2, "bcd", "bcd" + 2) == "abcbc" + 2);
		CHECK(ext::strrfind("abcbc", "abcbc" + 3, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strrfind("abcbc", "abcbc" + 4, "bcd", "bcd" + 2) == "abcbc" + 1);
		CHECK(ext::strrfind("abcbc", "abcbc" + 5, "bcd", "bcd" + 2) == "abcbc" + 3);

		CHECK(ext::strrfind("abcbc", "abcbc" + 0, "bcd", "bcd" + 3) == "abcbc" + 0);
		CHECK(ext::strrfind("abcbc", "abcbc" + 1, "bcd", "bcd" + 3) == "abcbc" + 1);
		CHECK(ext::strrfind("abcbc", "abcbc" + 2, "bcd", "bcd" + 3) == "abcbc" + 2);
		CHECK(ext::strrfind("abcbc", "abcbc" + 3, "bcd", "bcd" + 3) == "abcbc" + 3);
		CHECK(ext::strrfind("abcbc", "abcbc" + 4, "bcd", "bcd" + 3) == "abcbc" + 4);
		CHECK(ext::strrfind("abcbc", "abcbc" + 5, "bcd", "bcd" + 3) == "abcbc" + 5);
	}
}
