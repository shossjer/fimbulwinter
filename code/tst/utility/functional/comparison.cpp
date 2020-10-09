#include "utility/functional/comparison.hpp"

#include <catch2/catch.hpp>

TEST_CASE("", "[utility][functional]")
{
	CHECK((fun::equal_to(fun::_1, 2))(2) == true);
	CHECK((fun::equal_to(2, fun::_1))(2) == true);
	CHECK((fun::equal_to(fun::_1, 2))(3) == false);
	CHECK((fun::equal_to(2, fun::_1))(3) == false);

	CHECK((fun::equal_to(fun::_1, 3) | fun::equal_to(fun::_1, true))(3));
	CHECK((fun::equal_to(fun::_1, 3) | fun::equal_to(fun::_1, false))(2));
	CHECK((fun::equal_to(fun::_1, 3) | fun::equal_to(true, fun::_1))(3));
	CHECK((fun::equal_to(fun::_1, 3) | fun::equal_to(false, fun::_1))(2));
	CHECK((fun::equal_to(3, fun::_1) | fun::equal_to(fun::_1, true))(3));
	CHECK((fun::equal_to(3, fun::_1) | fun::equal_to(fun::_1, false))(2));
	CHECK((fun::equal_to(3, fun::_1) | fun::equal_to(true, fun::_1))(3));
	CHECK((fun::equal_to(3, fun::_1) | fun::equal_to(false, fun::_1))(2));
}
