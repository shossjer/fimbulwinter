#include "utility/bitmanip.hpp"

#include <catch2/catch.hpp>

TEST_CASE("clp2", "[utility][bitmanip]")
{
	CHECK(utility::clp2(uint32_t(0)) == 0);
	CHECK(utility::clp2(uint32_t(1)) == 1);
	CHECK(utility::clp2(uint32_t(2)) == 2);
	CHECK(utility::clp2(uint32_t(3)) == 4);
	CHECK(utility::clp2(uint32_t(0x7fffffff)) == 0x80000000);
	CHECK(utility::clp2(uint32_t(0x80000000)) == 0x80000000);
	CHECK(utility::clp2(uint32_t(0x80000001)) == 0);
	CHECK(utility::clp2(uint32_t(0xffffffff)) == 0);

	CHECK(utility::clp2(uint64_t(0)) == 0);
	CHECK(utility::clp2(uint64_t(1)) == 1);
	CHECK(utility::clp2(uint64_t(2)) == 2);
	CHECK(utility::clp2(uint64_t(3)) == 4);
	CHECK(utility::clp2(uint64_t(0x7fffffffffffffff)) == 0x8000000000000000);
	CHECK(utility::clp2(uint64_t(0x8000000000000000)) == 0x8000000000000000);
	CHECK(utility::clp2(uint64_t(0x8000000000000001)) == 0);
	CHECK(utility::clp2(uint64_t(0xffffffffffffffff)) == 0);
}
