#include "engine/Asset.hpp"

#include <catch2/catch.hpp>

TEST_CASE("A zero initialized Asset", "[engine][asset]")
{
	engine::Asset asset{};

	SECTION("contains the underlying value zero")
	{
		CHECK(static_cast<engine::Asset::value_type>(asset) == 0);
	}

	SECTION("is the same an empty string Asset")
	{
		CHECK(asset == engine::Asset(ful::cstr_utf8("")));
	}
}
