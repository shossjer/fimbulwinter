#include "utility/null_allocator.hpp"

#include <catch2/catch.hpp>

TEST_CASE("null allocator has default constructor", "[allocator][utility]")
{
	utility::null_allocator<char> na;
	fiw_unused(na);
}

TEST_CASE("null allocator fails to allocate", "[allocator][utility]")
{
	utility::null_allocator<char> na;

	CHECK_FALSE(na.allocate(0));
}
