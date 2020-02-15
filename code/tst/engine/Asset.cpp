#include "engine/Asset.hpp"

#include <catch/catch.hpp>

TEST_CASE("A zero initialized Asset", "[engine][asset]")
{
	engine::Asset asset{};

	SECTION("contains the underlying value zero")
	{
		CHECK(static_cast<uint64_t>(asset) == 0);
	}

	SECTION("is the same an empty string Asset")
	{
		CHECK(asset == engine::Asset(""));
	}
}
