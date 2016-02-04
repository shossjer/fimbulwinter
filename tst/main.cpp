
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE( "Important test", "Make sure logic works" )
{
    REQUIRE((1 && 2));
	REQUIRE_FALSE((1 & 2));

	REQUIRE((1 || 2));
	REQUIRE((1 | 2));
	REQUIRE((1 | 2) == (1 | 3));
}
