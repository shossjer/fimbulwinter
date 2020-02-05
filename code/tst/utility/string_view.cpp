#include "utility/string_view.hpp"

#include <catch/catch.hpp>

#include <sstream>

TEST_CASE("", "[utility][string view]")
{
	CHECK(utility::string_view("\"hej") == "\"hej");
}

TEST_CASE("", "[utility][string view]")
{
	std::stringstream ss;
	ss << utility::string_view("hello");
	ss << utility::string_view(", captain pants", 9);
	CHECK(ss.str() == "hello, captain");
}
