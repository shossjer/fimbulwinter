#include "utility/unicode/string_view.hpp"

#include <catch2/catch.hpp>

TEST_CASE("", "[utility][unicode][string]")
{
	utility::string_units_utf8 a = u8"\U0001f602\u2603$\u05d0\U0001f602\u2603\u05d0$";

	CHECK(find(a, '$').get() == (a.begin() + 7).get());
	CHECK(rfind(a, '$').get() == (a.begin() + 19).get());

	CHECK(find(find(a, '$') + 1, a.end(), '$').get() == (a.begin() + 19).get());
	CHECK(rfind(a.begin(), rfind(a, '$'), '$').get() == (a.begin() + 7).get());

	CHECK(find(a, u8"\u2603").get() == (a.begin() + 4).get());
	CHECK(rfind(a, u8"\u2603").get() == (a.begin() + 14).get());

	CHECK(find(find(a, u8"\u2603") + 1, a.end(), u8"\u2603").get() == (a.begin() + 14).get());
	CHECK(rfind(a.begin(), rfind(a, u8"\u2603"), u8"\u2603").get() == (a.begin() + 4).get());
}

TEST_CASE("", "[utility][unicode][string]")
{
	utility::string_points_utf8 a = u8"\U0001f602\u2603$\u05d0\U0001f602\u2603\u05d0$";

	CHECK(find(a, '$').get() == (a.begin() + 2).get());
	CHECK(rfind(a, '$').get() == (a.begin() + 7).get());

	CHECK(find(find(a, '$') + 1, a.end(), '$').get() == (a.begin() + 7).get());
	CHECK(rfind(a.begin(), rfind(a, '$'), '$').get() == (a.begin() + 2).get());

	CHECK(find(a, u'\u2603').get() == (a.begin() + 1).get());
	CHECK(rfind(a, u'\u2603').get() == (a.begin() + 5).get());

	CHECK(find(find(a, u'\u2603') + 1, a.end(), u'\u2603').get() == (a.begin() + 5).get());
	CHECK(rfind(a.begin(), rfind(a, u'\u2603'), u'\u2603').get() == (a.begin() + 1).get());
}
