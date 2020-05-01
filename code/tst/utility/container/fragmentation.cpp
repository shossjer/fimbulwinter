#include "utility/container/fragmentation.hpp"

#include <catch/catch.hpp>

TEST_CASE("", "")
{
	utility::static_fragmentation<3, int> c;

	int * const v1 = c.try_emplace(1);
	CHECK(v1);

	int * const v2 = c.try_emplace(2);
	CHECK(v2);
	REQUIRE(v2 != v1);

	int * const v3 = c.try_emplace(3);
	CHECK(v3);
	REQUIRE(v3 != v1);
	REQUIRE(v3 != v2);

	SECTION("")
	{
		CHECK_FALSE(c.try_emplace(4));
	}

	SECTION("")
	{
		CHECK(c.try_erase(0));

		int * const v4 = c.try_emplace(4);
		CHECK(v4);
		CHECK(v4 == v1);
	}

	SECTION("")
	{
		CHECK(c.try_erase(2));
		CHECK(c.try_erase(1));
		CHECK(c.try_erase(0));
	}
}
