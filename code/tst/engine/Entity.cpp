#include "engine/Entity.hpp"

#include <catch2/catch.hpp>

TEST_CASE("A zero initialized Entity", "[engine][entity]")
{
	const auto entity = engine::Entity{};

	SECTION("contains zero")
	{
		CHECK(entity.value() == 0);
	}
}
