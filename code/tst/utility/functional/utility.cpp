#include "utility/functional/utility.hpp"

#include <catch2/catch.hpp>

TEST_CASE("", "[utility][functional]")
{
	CHECK(fun::first(std::make_pair(2, 3)) == 2);

	CHECK((fun::first == 2)(std::make_pair(2, 3)) == true);
	CHECK((2 == fun::first)(std::make_pair(2, 3)) == true);
}
