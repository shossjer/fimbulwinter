#include "engine/Entity.hpp"
#include "engine/EntityFactory.hpp"

#include <catch2/catch.hpp>

TEST_CASE("A zero initialized Entity", "[engine][entity]")
{
	const auto entity = engine::Entity{};

	SECTION("contains zero")
	{
		CHECK(entity.value() == 0);
	}
}

TEST_CASE("A default constructed EntityFactory", "[engine][entity]")
{
	engine::EntityFactory factory;

	SECTION("starts counting from 1")
	{
		CHECK(factory.create().value() == 1);
		CHECK(factory.create().value() == 2);
		CHECK(factory.create().value() == 3);
	}
}

TEST_CASE("A value-constructed EntityFactory", "[engine][entity]")
{
	engine::EntityFactory factory(123456789);

	SECTION("starts counting from the given value")
	{
		CHECK(factory.create().value() == 123456789);
		CHECK(factory.create().value() == 123456790);
		CHECK(factory.create().value() == 123456791);
	}
}
