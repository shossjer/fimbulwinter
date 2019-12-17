
#include "catch.hpp"

#include "utility/null_allocator.hpp"

TEST_CASE("null allocator has default constructor", "[allocator][utility]")
{
	utility::null_allocator<char> na;
}

TEST_CASE("null allocator fails to allocate", "[allocator][utility]")
{
	utility::null_allocator<char> na;

	CHECK_FALSE(na.allocate(0));
}
