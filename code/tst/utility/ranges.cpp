#include <catch.hpp>

#include "utility/ranges.hpp"
#include "utility/type_traits.hpp"

#include <array>

TEST_CASE("index sequence range goes from low to high", "[utility][ranges]")
{
	const int expected_values[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

	int count = 0;
	for (auto value : ranges::index_sequence(11))
	{
		REQUIRE(value == expected_values[count]);
		count++;
	}
	CHECK(count == 11);
}

TEST_CASE("index sequence range for containers", "[utility][ranges]")
{
	const int expected_values[] = { 0, 1, 2, 3, 4, 5, 6 };

	int count = 0;
	for (auto value : ranges::index_sequence_for(std::array<int, 7>{}))
	{
		REQUIRE(value == expected_values[count]);
		count++;
	}
	CHECK(count == 7);
}
