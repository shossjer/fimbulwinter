
#include "catch.hpp"

#include <utility/iterator.hpp>

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
