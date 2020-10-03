#include "utility/charconv.hpp"

#include <catch2/catch.hpp>

TEST_CASE("from string to signed 8 bit integer", "[utility][charconv]")
{
  std::int8_t value = -1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("127", "127" + 3, value).ec == std::errc{});
	CHECK(value == 127);

	CHECK(ext::from_chars("128", "128" + 3, value).ec == std::errc::result_out_of_range);
	CHECK(value == 127);

	CHECK(ext::from_chars("200", "200" + 3, value).ec == std::errc::result_out_of_range);
	CHECK(value == 127);

	CHECK(ext::from_chars("1000", "1000" + 4, value).ec == std::errc::result_out_of_range);
	CHECK(value == 127);

	CHECK(ext::from_chars("-128", "-128" + 4, value).ec == std::errc{});
	CHECK(value == -127 - 1);

	CHECK(ext::from_chars("-129", "-129" + 4, value).ec == std::errc::result_out_of_range);
	CHECK(value == -127 - 1);

	CHECK(ext::from_chars("-200", "-200" + 4, value).ec == std::errc::result_out_of_range);
	CHECK(value == -127 - 1);

	CHECK(ext::from_chars("-1000", "-1000" + 5, value).ec == std::errc::result_out_of_range);
	CHECK(value == -127 - 1);
}

TEST_CASE("from string to signed 16 bit integer", "[utility][charconv]")
{
  std::int16_t value = -1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("32767", "32767" + 5, value).ec == std::errc{});
	CHECK(value == 32767);

	CHECK(ext::from_chars("32768", "32768" + 5, value).ec == std::errc::result_out_of_range);
	CHECK(value == 32767);

	CHECK(ext::from_chars("40000", "30000" + 5, value).ec == std::errc::result_out_of_range);
	CHECK(value == 32767);

	CHECK(ext::from_chars("100000", "100000" + 6, value).ec == std::errc::result_out_of_range);
	CHECK(value == 32767);

	CHECK(ext::from_chars("-32768", "-32768" + 6, value).ec == std::errc{});
	CHECK(value == -32767 - 1);

	CHECK(ext::from_chars("-32769", "-32769" + 6, value).ec == std::errc::result_out_of_range);
	CHECK(value == -32767 - 1);

	CHECK(ext::from_chars("-40000", "-40000" + 6, value).ec == std::errc::result_out_of_range);
	CHECK(value == -32767 - 1);

	CHECK(ext::from_chars("-100000", "-100000" + 7, value).ec == std::errc::result_out_of_range);
	CHECK(value == -32767 - 1);
}

TEST_CASE("from string to signed 32 bit integer", "[utility][charconv]")
{
  std::int32_t value = -1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("2147483647", "2147483647" + 10, value).ec == std::errc{});
	CHECK(value == 2147483647);

	CHECK(ext::from_chars("2147483648", "2147483648" + 10, value).ec == std::errc::result_out_of_range);
	CHECK(value == 2147483647);

	CHECK(ext::from_chars("3000000000", "3000000000" + 10, value).ec == std::errc::result_out_of_range);
	CHECK(value == 2147483647);

	CHECK(ext::from_chars("10000000000", "10000000000" + 11, value).ec == std::errc::result_out_of_range);
	CHECK(value == 2147483647);

	CHECK(ext::from_chars("-2147483648", "-2147483648" + 11, value).ec == std::errc{});
	CHECK(value == -2147483647 - 1);

	CHECK(ext::from_chars("-2147483649", "-2147483649" + 11, value).ec == std::errc::result_out_of_range);
	CHECK(value == -2147483647 - 1);

	CHECK(ext::from_chars("-3000000000", "-3000000000" + 11, value).ec == std::errc::result_out_of_range);
	CHECK(value == -2147483647 - 1);

	CHECK(ext::from_chars("-10000000000", "-10000000000" + 12, value).ec == std::errc::result_out_of_range);
	CHECK(value == -2147483647 - 1);
}

TEST_CASE("from string to signed 64 bit integer", "[utility][charconv]")
{
  std::int64_t value = -1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("9223372036854775807", "9223372036854775807" + 19, value).ec == std::errc{});
	CHECK(value == 9223372036854775807ll);

	CHECK(ext::from_chars("9223372036854775808", "9223372036854775808" + 19, value).ec == std::errc::result_out_of_range);
	CHECK(value == 9223372036854775807ll);

	CHECK(ext::from_chars("10000000000000000000", "10000000000000000000" + 20, value).ec == std::errc::result_out_of_range);
	CHECK(value == 9223372036854775807ll);

	CHECK(ext::from_chars("100000000000000000000", "100000000000000000000" + 21, value).ec == std::errc::result_out_of_range);
	CHECK(value == 9223372036854775807ll);

	CHECK(ext::from_chars("-9223372036854775808", "-9223372036854775808" + 20, value).ec == std::errc{});
	CHECK(value == -9223372036854775807ll - 1);

	CHECK(ext::from_chars("-9223372036854775809", "-9223372036854775809" + 20, value).ec == std::errc::result_out_of_range);
	CHECK(value == -9223372036854775807ll - 1);

	CHECK(ext::from_chars("-10000000000000000000", "-10000000000000000000" + 21, value).ec == std::errc::result_out_of_range);
	CHECK(value == -9223372036854775807ll - 1);
}

TEST_CASE("from string to unsigned 8 bit integer", "[utility][charconv]")
{
  std::uint8_t value = 1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("255", "255" + 3, value).ec == std::errc{});
	CHECK(value == 255);

	CHECK(ext::from_chars("256", "256" + 3, value).ec == std::errc::result_out_of_range);
	CHECK(value == 255);

	CHECK(ext::from_chars("300", "300" + 3, value).ec == std::errc::result_out_of_range);
	CHECK(value == 255);

	CHECK(ext::from_chars("1000", "1000" + 4, value).ec == std::errc::result_out_of_range);
	CHECK(value == 255);
}

TEST_CASE("from string to unsigned 16 bit integer", "[utility][charconv]")
{
  std::uint16_t value = 1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("65535", "65535" + 5, value).ec == std::errc{});
	CHECK(value == 65535);

	CHECK(ext::from_chars("65536", "65536" + 5, value).ec == std::errc::result_out_of_range);
	CHECK(value == 65535);

	CHECK(ext::from_chars("70000", "70000" + 5, value).ec == std::errc::result_out_of_range);
	CHECK(value == 65535);

	CHECK(ext::from_chars("100000", "100000" + 6, value).ec == std::errc::result_out_of_range);
	CHECK(value == 65535);
}

TEST_CASE("from string to unsigned 32 bit integer", "[utility][charconv]")
{
  std::uint32_t value = 1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("4294967295", "4294967295" + 10, value).ec == std::errc{});
	CHECK(value == 4294967295);

	CHECK(ext::from_chars("4294967296", "4294967296" + 10, value).ec == std::errc::result_out_of_range);
	CHECK(value == 4294967295);

	CHECK(ext::from_chars("5000000000", "5000000000" + 10, value).ec == std::errc::result_out_of_range);
	CHECK(value == 4294967295);

	CHECK(ext::from_chars("10000000000", "10000000000" + 11, value).ec == std::errc::result_out_of_range);
	CHECK(value == 4294967295);
}

TEST_CASE("from string to unsigned 64 bit integer", "[utility][charconv]")
{
  std::uint64_t value = 1;

	CHECK(ext::from_chars("0", "0" + 1, value).ec == std::errc{});
	CHECK(value == 0);

	CHECK(ext::from_chars("18446744073709551615", "18446744073709551615" + 20, value).ec == std::errc{});
	CHECK(value == 18446744073709551615ull);

	CHECK(ext::from_chars("18446744073709551616", "18446744073709551616" + 20, value).ec == std::errc::result_out_of_range);
	CHECK(value == 18446744073709551615ull);

	CHECK(ext::from_chars("20000000000000000000", "20000000000000000000" + 20, value).ec == std::errc::result_out_of_range);
	CHECK(value == 18446744073709551615ull);

	CHECK(ext::from_chars("100000000000000000000", "100000000000000000000" + 21, value).ec == std::errc::result_out_of_range);
	CHECK(value == 18446744073709551615ull);
}
