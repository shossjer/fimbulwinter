#include "core/debug.hpp"

#include <catch/catch.hpp>

namespace
{
	struct A
	{
		int i;

		A() = delete;
		A(int i) : i(i) {}
		A(const A &) = delete;
		A(A &&) = delete;

		friend bool operator == (const A & a1, const A & a2)
		{
			return a1.i == a2.i;
		}
	};
}

TEST_CASE( "debug_assert", "[utility]" )
{
	debug_assert(1 + 1 == 2);
	debug_assert(1 < 2);

	// Visual Studio say nope
	//debug_assert(std::string("hola") == "hola");

	debug_assert(A{1} == A{1});

	debug_assert(true);
	debug_assert((3 == 3));
	debug_assert(nullptr == nullptr);

	const int a = 7;
	const int b = 7;
	debug_assert(a == b);

	debug_assert(1 != 2);
	debug_assert(a < 11);
	debug_assert(b <= 7);
	debug_assert(a > 3);
	debug_assert(8 >= 7);

	debug_assert(3 == 7 >> 1, "it should be three!");
}

TEST_CASE("debug cast", "[core][debug]")
{
	SECTION("is fine between signed/unsigned of same size")
	{
		// no data is lost
		CHECK(debug_cast<int32_t>(uint32_t{0x80000000u}) == int32_t{-0x7fffffff - 1});

		// we can even do a round trip
		CHECK(debug_cast<uint32_t>(debug_cast<int32_t>(uint32_t{0x80000000u})) == uint32_t{0x80000000u});
	}

	SECTION("is fine to bigger sized types")
	{
		// SECTION("from
		CHECK(debug_cast<int16_t>(int8_t{-0x7f - 1}) == int8_t{-0x7f - 1});
		CHECK(debug_cast<int32_t>(int16_t{-0x7fff - 1}) == int16_t{-0x7fff - 1});
		CHECK(debug_cast<int64_t>(int32_t{-0x7fffffff - 1}) == int32_t{-0x7fffffff - 1});
	}

	// SECTION("fails when data is lost")
	// {
	// 	CHECK_FALSE(debug_cast<uint32_t>(0x100000000ull));
	// }
}
