#include "utility/algorithm.hpp"
#include "utility/ranges.hpp"

#include <catch2/catch.hpp>

TEST_CASE("to_array", "[utility][algorithm]")
{
	SECTION("handles lvalue non-const int values")
	{
		int values[] = {2, 3, 5, 7, 11};
		const auto result = utl::to_array(values);

		const std::array<int, 5> expected = {
			2, 3, 5, 7, 11
		};
		CHECK(result == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(result[i] == expected[i]);
		}
	}

	SECTION("handles lvalue const int values")
	{
		const int values[] = {2, 3, 5, 7, 11};
		const auto result = utl::to_array(values);

		const std::array<int, 5> expected = {
			2, 3, 5, 7, 11
		};
		CHECK(result == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(result[i] == expected[i]);
		}
	}

	SECTION("handles rvalue non-const int values")
	{
		int values[] = {2, 3, 5, 7, 11};
		const auto result = utl::to_array(std::move(values));

		const std::array<int, 5> expected = {
			2, 3, 5, 7, 11
		};
		CHECK(result == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(result[i] == expected[i]);
		}
	}

	SECTION("handles rvalue const int values")
	{
		const int values[] = {2, 3, 5, 7, 11};
		const auto result = utl::to_array(std::move(values));

		const std::array<int, 5> expected = {
			2, 3, 5, 7, 11
		};
		CHECK(result == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(result[i] == expected[i]);
		}
	}
}

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

TEST_CASE("make_table", "[utility][algorithm]")
{
	SECTION("handles ints")
	{
		const auto result = utl::make_table<10>(std::make_pair(2, 1),
		                                        std::make_pair(3, 2),
		                                        std::make_pair(5, 3),
		                                        std::make_pair(7, 4));

		const std::array<int, 10> expected = {
			0, 0, 1, 2, 0, 3, 0, 4, 0, 0
		};
		CHECK(result == expected);
		for (auto i : ranges::index_sequence_for(expected))
		{
			REQUIRE(result[i] == expected[i]);
		}
	}
}
