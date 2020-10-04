#include "utility/overflow.hpp"

#include <catch2/catch.hpp>

TEST_CASE("overflow when adding signed 8 bit integers", "[utility][overflow]")
{
	signed char result = 0;

	CHECK_FALSE(utility::add_overflow(1, 0, &result));
	CHECK((int)result == 1);
	CHECK_FALSE(utility::add_overflow(127, 0, &result));
	CHECK((int)result == 127);
	CHECK_FALSE(utility::add_overflow(-1, 0, &result));
	CHECK((int)result == -1);
	CHECK_FALSE(utility::add_overflow(-128, 0, &result));
	CHECK((int)result == -128);
	CHECK_FALSE(utility::add_overflow(0, 1, &result));
	CHECK((int)result == 1);
	CHECK_FALSE(utility::add_overflow(0, 127, &result));
	CHECK((int)result == 127);
	CHECK_FALSE(utility::add_overflow(0, -1, &result));
	CHECK((int)result == -1);
	CHECK_FALSE(utility::add_overflow(0, -128, &result));
	CHECK((int)result == -128);

	CHECK(utility::add_overflow(127, 1, &result));
	CHECK(utility::add_overflow(127, 127, &result));
	CHECK(utility::add_overflow(1, 127, &result));

	CHECK(utility::add_overflow(-128, -1, &result));
	CHECK(utility::add_overflow(-128, -128, &result));
	CHECK(utility::add_overflow(-1, -128, &result));

	CHECK_FALSE(utility::add_overflow(127, -127, &result));
	CHECK((int)result == 0);
	CHECK_FALSE(utility::add_overflow(127, -128, &result));
	CHECK((int)result == -1);
	CHECK_FALSE(utility::add_overflow(-127, 127, &result));
	CHECK((int)result == 0);
	CHECK_FALSE(utility::add_overflow(-128, 127, &result));
	CHECK((int)result == -1);
}

TEST_CASE("", "[utility][overflow]")
{
	signed char result = 0;

	CHECK_FALSE(utility::mul_overflow(1, 127, &result));
	CHECK((int)result == 127);
	CHECK_FALSE(utility::mul_overflow(0, 127, &result));
	CHECK((int)result == 0);
	CHECK_FALSE(utility::mul_overflow(1, -128, &result));
	CHECK((int)result == -128);
	CHECK_FALSE(utility::mul_overflow(0, -128, &result));
	CHECK((int)result == 0);
	CHECK_FALSE(utility::mul_overflow(127, 1, &result));
	CHECK((int)result == 127);
	CHECK_FALSE(utility::mul_overflow(127, 0, &result));
	CHECK((int)result == 0);
	CHECK_FALSE(utility::mul_overflow(-128, 1, &result));
	CHECK((int)result == -128);
	CHECK_FALSE(utility::mul_overflow(-128, 0, &result));
	CHECK((int)result == 0);

	CHECK(utility::mul_overflow(2, 100, &result));
	CHECK(utility::mul_overflow(-2, 100, &result));
	CHECK(utility::mul_overflow(2, -100, &result));
	CHECK(utility::mul_overflow(-2, -100, &result));
}
