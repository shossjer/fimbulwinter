
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "core/debug.hpp"

// todo : extend catch's console reporter to print a better message when
// asserts fail
//
// https://github.com/catchorg/Catch2/blob/master/include/reporters/catch_reporter_console.cpp#L691

namespace
{
	bool test_fail_hook()
	{
		FAIL("assert failed, see error above");
		return false;
	}

	struct setup_debug
	{
		setup_debug()
		{
			core::debug::instance().set_mask(0); // turn off printlines
			core::debug::instance().set_fail_hook(test_fail_hook);
		}
	} setup_debug_hack;
}

TEST_CASE( "Important test", "Make sure logic works" )
{
    REQUIRE((1 && 2));
	REQUIRE_FALSE((1 & 2));

	REQUIRE((1 || 2));
	REQUIRE((1 | 2));
	REQUIRE((1 | 2) == (1 | 3));
}
