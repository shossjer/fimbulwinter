#include <catch/catch.hpp>

#include "utility/string_conversions.hpp"

TEST_CASE("the size of a char string", "[encoding][string][utility]")
{
	constexpr utility::basic_string_view<char> char_view = "123456789";

	SECTION("in terms of char")
	{
		constexpr auto size = utility::size<char>(char_view);
		CHECK(size == 9);
	}

	SECTION("in terms of char16_t")
	{
		constexpr auto size = utility::size<char16_t>(char_view);
		CHECK(size == 9);
	}

	SECTION("in terms of char32_t")
	{
		constexpr auto size = utility::size<char32_t>(char_view);
		CHECK(size == 9);
	}

#if defined(_MSC_VER) && defined(_UNICODE)
	SECTION("in terms of wchar_t")
	{
		constexpr auto size = utility::size<wchar_t>(char_view);
		CHECK(size == 9);
	}
#endif
}

TEST_CASE("", "[encoding][string][utility]")
{
	constexpr utility::basic_string_view<char> char_view = "123456789";

	SECTION("")
	{
		char buffer[9] = {};
		utility::convert(char_view, buffer, 9);

		CHECK(buffer[0] == char_view[0]);
		CHECK(buffer[1] == char_view[1]);
		CHECK(buffer[2] == char_view[2]);
		CHECK(buffer[3] == char_view[3]);
		CHECK(buffer[4] == char_view[4]);
		CHECK(buffer[5] == char_view[5]);
		CHECK(buffer[6] == char_view[6]);
		CHECK(buffer[7] == char_view[7]);
		CHECK(buffer[8] == char_view[8]);
	}
}
