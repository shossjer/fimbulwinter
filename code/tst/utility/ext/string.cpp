#include "utility/ext/string.hpp"

#include <catch2/catch.hpp>

TEST_CASE("strend", "[ext][string][utility]")
{
	SECTION("stops at null terminator")
	{
		{
			const char * str = "";
			CHECK(ext::strend(str) == str + 0);
		}
		{
			const char * str = "123";
			CHECK(ext::strend(str) == str + 3);
		}
	}

	SECTION("stops at arbitrary delimiter")
	{
		{
			const char * str = "a";
			CHECK(ext::strend(str, 'a') == str + 0);
		}
		{
			const char * str = "bbbabb";
			CHECK(ext::strend(str, 'a') == str + 3);
		}
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
		const char * str = "ab";
		CHECK(ext::strmatch(str, str + 0, "aa"));
		CHECK(ext::strmatch(str, str + 1, "aa"));
		CHECK_FALSE(ext::strmatch(str, str + 2, "aa"));

		CHECK(ext::strmatch(str, str + 0, "bb"));
		CHECK_FALSE(ext::strmatch(str, str + 1, "bb"));
		CHECK_FALSE(ext::strmatch(str, str + 2, "bb"));
	}
}

TEST_CASE("strbegins", "[ext][string][utility]")
{
	SECTION("checks if range begins with character")
	{
		const char * str = "ab";
		CHECK_FALSE(ext::strbegins(str, str + 0, 'a'));
		CHECK(ext::strbegins(str, str + 1, 'a'));
		CHECK(ext::strbegins(str, str + 2, 'a'));

		CHECK_FALSE(ext::strbegins(str, str + 0, 'b'));
		CHECK_FALSE(ext::strbegins(str, str + 1, 'b'));
		CHECK_FALSE(ext::strbegins(str, str + 2, 'b'));
	}

	SECTION("checks if range begins with string")
	{
		{
			const char * str = "a";
			CHECK(ext::strbegins(str, str + 0, ""));
			CHECK(ext::strbegins(str, str + 1, ""));
		}
		{
			const char * str = "abc";
			CHECK_FALSE(ext::strbegins(str, str + 0, "ab"));
			CHECK_FALSE(ext::strbegins(str, str + 1, "ab"));
			CHECK(ext::strbegins(str, str + 2, "ab"));
			CHECK(ext::strbegins(str, str + 3, "ab"));

			CHECK_FALSE(ext::strbegins(str, str + 0, "abb"));
			CHECK_FALSE(ext::strbegins(str, str + 1, "abb"));
			CHECK_FALSE(ext::strbegins(str, str + 2, "abb"));
			CHECK_FALSE(ext::strbegins(str, str + 3, "abb"));
		}
	}

	SECTION("checks if range begins like another range")
	{
		{
			const char * str1 = "a";
			const char * str2 = "b";
			CHECK(ext::strbegins(str1, str1 + 0, str2, str2 + 0));
			CHECK(ext::strbegins(str1, str1 + 1, str2, str2 + 0));
			CHECK_FALSE(ext::strbegins(str1, str1 + 0, str1, str1 + 1));
			CHECK(ext::strbegins(str1, str1 + 1, str1, str1 + 1));
		}
		{
			const char * str1 = "abc";
			const char * str2 = "abb";
			CHECK_FALSE(ext::strbegins(str1, str1 + 0, str2, str2 + 2));
			CHECK_FALSE(ext::strbegins(str1, str1 + 1, str2, str2 + 2));
			CHECK(ext::strbegins(str1, str1 + 2, str2, str2 + 2));
			CHECK(ext::strbegins(str1, str1 + 3, str2, str2 + 2));

			CHECK_FALSE(ext::strbegins(str1, str1 + 0, str2, str2 + 3));
			CHECK_FALSE(ext::strbegins(str1, str1 + 1, str2, str2 + 3));
			CHECK_FALSE(ext::strbegins(str1, str1 + 2, str2, str2 + 3));
			CHECK_FALSE(ext::strbegins(str1, str1 + 3, str2, str2 + 3));
		}
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
		const char * str = "ab";
		CHECK(ext::strcmp(str, str + 0, 'a') < 0);
		CHECK(ext::strcmp(str, str + 1, 'a') == 0);
		CHECK(ext::strcmp(str, str + 2, 'a') > 0);

		CHECK(ext::strcmp(str, str + 0, 'b') < 0);
		CHECK(ext::strcmp(str, str + 1, 'b') < 0);
		CHECK(ext::strcmp(str, str + 2, 'b') < 0);
	}

	SECTION("checks how range compares to string")
	{
		{
			const char * str = "a";
			CHECK(ext::strcmp(str, str + 0, "") == 0);
			CHECK(ext::strcmp(str, str + 1, "") > 0);
		}
		{
			const char * str = "abc";
			CHECK(ext::strcmp(str, str + 0, "ab") < 0);
			CHECK(ext::strcmp(str, str + 1, "ab") < 0);
			CHECK(ext::strcmp(str, str + 2, "ab") == 0);
			CHECK(ext::strcmp(str, str + 3, "ab") > 0);

			CHECK(ext::strcmp(str, str + 0, "abb") < 0);
			CHECK(ext::strcmp(str, str + 1, "abb") < 0);
			CHECK(ext::strcmp(str, str + 2, "abb") < 0);
			CHECK(ext::strcmp(str, str + 3, "abb") > 0);
		}
	}

	SECTION("checks how range compares to another range")
	{
		{
			const char * str1 = "a";
			const char * str2 = "b";
			CHECK(ext::strcmp(str1, str1 + 0, str2, str2 + 0) == 0);
			CHECK(ext::strcmp(str1, str1 + 1, str2, str2 + 0) > 0);
			CHECK(ext::strcmp(str1, str1 + 0, str1, str1 + 1) < 0);
			CHECK(ext::strcmp(str1, str1 + 1, str1, str1 + 1) == 0);
		}
		{
			const char * str1 = "abc";
			const char * str2 = "abb";
			CHECK(ext::strcmp(str1, str1 + 0, str2, str2 + 2) < 0);
			CHECK(ext::strcmp(str1, str1 + 1, str2, str2 + 2) < 0);
			CHECK(ext::strcmp(str1, str1 + 2, str2, str2 + 2) == 0);
			CHECK(ext::strcmp(str1, str1 + 3, str2, str2 + 2) > 0);

			CHECK(ext::strcmp(str1, str1 + 0, str2, str2 + 3) < 0);
			CHECK(ext::strcmp(str1, str1 + 1, str2, str2 + 3) < 0);
			CHECK(ext::strcmp(str1, str1 + 2, str2, str2 + 3) < 0);
			CHECK(ext::strcmp(str1, str1 + 3, str2, str2 + 3) > 0);
		}
	}
}

TEST_CASE("strfind", "[ext][string][utility]")
{
	SECTION("finds first occurrence of character")
	{
		{
			const char * str = "aba";
			CHECK(ext::strfind(str, str + 0, 'a') == str + 0);
			CHECK(ext::strfind(str, str + 1, 'a') == str + 0);
			CHECK(ext::strfind(str, str + 2, 'a') == str + 0);
			CHECK(ext::strfind(str, str + 3, 'a') == str + 0);
		}
		{
			const char * str = "abb";
			CHECK(ext::strfind(str, str + 0, 'b') == str + 0);
			CHECK(ext::strfind(str, str + 1, 'b') == str + 1);
			CHECK(ext::strfind(str, str + 2, 'b') == str + 1);
			CHECK(ext::strfind(str, str + 3, 'b') == str + 1);
		}
	}

	SECTION("finds first occurrence of another range")
	{
		{
			const char * str1 = "aa";
			const char * str2 = "a";
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 1) == str1 + 0);
		}
		{
			const char * str1 = "aa";
			const char * str2 = "b";
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 1) == str1 + 1);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 1) == str1 + 2);
		}
		{
			const char * str1 = "abcbc";
			const char * str2 = "bcd";
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 2) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 2) == str1 + 2);
			CHECK(ext::strfind(str1, str1 + 3, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strfind(str1, str1 + 4, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strfind(str1, str1 + 5, str2, str2 + 2) == str1 + 1);
		}
		{
			const char * str1 = "abcbc";
			const char * str2 = "bcd";
			CHECK(ext::strfind(str1, str1 + 0, str2, str2 + 3) == str1 + 0);
			CHECK(ext::strfind(str1, str1 + 1, str2, str2 + 3) == str1 + 1);
			CHECK(ext::strfind(str1, str1 + 2, str2, str2 + 3) == str1 + 2);
			CHECK(ext::strfind(str1, str1 + 3, str2, str2 + 3) == str1 + 3);
			CHECK(ext::strfind(str1, str1 + 4, str2, str2 + 3) == str1 + 4);
			CHECK(ext::strfind(str1, str1 + 5, str2, str2 + 3) == str1 + 5);
		}
	}
}

TEST_CASE("strrfind", "[ext][string][utility]")
{
	SECTION("finds last occurrence of character")
	{
		{
			const char * str = "aba";
			CHECK(ext::strrfind(str, str + 0, 'a') == str + 0);
			CHECK(ext::strrfind(str, str + 1, 'a') == str + 0);
			CHECK(ext::strrfind(str, str + 2, 'a') == str + 0);
			CHECK(ext::strrfind(str, str + 3, 'a') == str + 2);
		}
		{
			const char * str = "abb";
			CHECK(ext::strrfind(str, str + 0, 'b') == str + 0);
			CHECK(ext::strrfind(str, str + 1, 'b') == str + 1);
			CHECK(ext::strrfind(str, str + 2, 'b') == str + 1);
			CHECK(ext::strrfind(str, str + 3, 'b') == str + 2);
		}
	}

	SECTION("finds last occurrence of another range")
	{
		{
			const char * str1 = "aa";
			const char * str2 = "a";
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 0) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 0) == str1 + 2);
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 1) == str1 + 1);
		}
		{
			const char * str1 = "aa";
			const char * str2 = "b";
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 0) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 0) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 0) == str1 + 2);
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 1) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 1) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 1) == str1 + 2);
		}
		{
			const char * str1 = "abcbc";
			const char * str2 = "bcd";
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 2) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 2) == str1 + 2);
			CHECK(ext::strrfind(str1, str1 + 3, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 4, str2, str2 + 2) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 5, str2, str2 + 2) == str1 + 3);
		}
		{
			const char * str1 = "abcbc";
			const char * str2 = "bcd";
			CHECK(ext::strrfind(str1, str1 + 0, str2, str2 + 3) == str1 + 0);
			CHECK(ext::strrfind(str1, str1 + 1, str2, str2 + 3) == str1 + 1);
			CHECK(ext::strrfind(str1, str1 + 2, str2, str2 + 3) == str1 + 2);
			CHECK(ext::strrfind(str1, str1 + 3, str2, str2 + 3) == str1 + 3);
			CHECK(ext::strrfind(str1, str1 + 4, str2, str2 + 3) == str1 + 4);
			CHECK(ext::strrfind(str1, str1 + 5, str2, str2 + 3) == str1 + 5);
		}
	}
}
