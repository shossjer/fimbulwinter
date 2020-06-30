#include "utility/iterator.hpp"

#include <catch2/catch.hpp>

#include <list>

TEST_CASE( "is_contiguous_iterator", "[utility]" )
{
	using utility::begin;

	static_assert(utility::is_contiguous_iterator<int*>::value, "");
	static_assert(utility::is_contiguous_iterator<const int*>::value, "");
	static_assert(utility::is_contiguous_iterator<std::move_iterator<int*>>::value, "");
	static_assert(utility::is_contiguous_iterator<std::move_iterator<const int*>>::value, "");
	static_assert(utility::is_contiguous_iterator<std::reverse_iterator<int*>>::value, "");
	static_assert(utility::is_contiguous_iterator<std::reverse_iterator<const int*>>::value, "");

	static_assert(utility::is_contiguous_iterator<std::move_iterator<std::reverse_iterator<int*>>>::value, "");
	static_assert(utility::is_contiguous_iterator<std::move_iterator<std::reverse_iterator<const int*>>>::value, "");

	static_assert(utility::is_contiguous_iterator<decltype(begin(std::declval<std::array<int, 10>>()))>::value, "");
	static_assert(utility::is_contiguous_iterator<decltype(begin(std::declval<std::valarray<int>>()))>::value, "");
	static_assert(utility::is_contiguous_iterator<decltype(begin(std::declval<std::vector<int>>()))>::value, "");

	static_assert(!utility::is_contiguous_iterator<decltype(begin(std::declval<std::list<int>>()))>::value, "");
}

TEST_CASE("reverse works on", "[utility]")
{
	SECTION("arrays")
	{
		const int values[] = {0, 1, 2, 3, 4};

		int count = 0;
		for (auto x : utility::reverse(values))
		{
			CHECK(x == values[4 - count]);
			count++;
		}
		REQUIRE(count == 5);
	}

	SECTION("vectors")
	{
		const std::vector<float> values = {0.f, 1.f, 2.f, 3.f, 4.f, 5.f, 6.f};

		int count = 0;
		for (auto x : utility::reverse(values))
		{
			CHECK(static_cast<float>(x) == values[6 - count]);
			count++;
		}
		REQUIRE(count == 7);
	}
}
