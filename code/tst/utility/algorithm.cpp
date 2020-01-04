#include <catch.hpp>

#include "utility/algorithm.hpp"
#include "utility/ranges.hpp"

TEST_CASE("inverse table", "[utility][algorithm]")
{
	SECTION("handles integers")
	{
		const int table[] = {17, 5, 13, 23, 3, 2, 11, 7};
		const auto inverse_table = utl::inverse_table<24>(table);

		const std::array<std::size_t, 24> expected = {
			8, 8, 5, 4, 8, 1, 8, 7, 8, 8, 8, 6, 8, 2, 8, 8, 8, 0, 8, 8, 8, 8, 8, 3
		};
		CHECK(inverse_table == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(inverse_table[i] == expected[i]);
		}
	}

	SECTION("ignores negative values")
	{
		const int table[] = {17, 5, -13, -23, 3, 2, 11, -7};
		const auto inverse_table = utl::inverse_table<24>(table);

		const std::array<std::size_t, 24> expected = {
			8, 8, 5, 4, 8, 1, 8, 8, 8, 8, 8, 6, 8, 8, 8, 8, 8, 0, 8, 8, 8, 8, 8, 8
		};
		CHECK(inverse_table == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(inverse_table[i] == expected[i]);
		}
	}

	SECTION("ignores too large values")
	{
		const int table[] = {17, 5, 13, 23, 3, 2, 11, 7};
		const auto inverse_table = utl::inverse_table<4>(table);

		const std::array<std::size_t, 4> expected = {
			8, 8, 5, 4
		};
		CHECK(inverse_table == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(inverse_table[i] == expected[i]);
		}
	}
}
