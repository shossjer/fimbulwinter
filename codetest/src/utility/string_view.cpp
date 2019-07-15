
#include "catch.hpp"

#include "utility/string_view.hpp"

TEST_CASE("")
{
	CHECK(utility::string_view("\"hej") == "\"hej");
}
