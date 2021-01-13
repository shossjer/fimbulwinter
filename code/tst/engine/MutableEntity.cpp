#include "engine/MutableEntity.hpp"

#include <catch2/catch.hpp>

TEST_CASE("mutable entities", "[engine][entity]")
{
	engine::MutableEntity ancestor(engine::Entity(0), 4711);

	SECTION("is ordered relative to other entities")
	{
		engine::Entity entity(1);

		CHECK_FALSE(ancestor == entity);
		CHECK(ancestor != entity);

		CHECK_FALSE(entity == ancestor);
		CHECK(entity != ancestor);

		engine::MutableEntity other(entity, 4711);

		CHECK_FALSE(ancestor == other);
		CHECK(ancestor != other);
	}

	SECTION("is ordered relative to mutations")
	{
		engine::MutableEntity mutation = ancestor;
		mutation.mutate();

		CHECK_FALSE(ancestor == mutation);
		CHECK(ancestor != mutation);

		engine::MutableEntity another = mutation;
		another.mutate();

		CHECK_FALSE(ancestor == another);
		CHECK(ancestor != another);

		CHECK_FALSE(mutation == another);
		CHECK(mutation != another);
	}

	SECTION("wraps around if mutated too much")
	{
		engine::MutableEntity mutation = ancestor;
		for (int i = INT32_MAX; 0 < i; i--)
		{
			mutation.mutate();
		}

		CHECK_FALSE(ancestor == mutation);
		CHECK(ancestor != mutation);

		mutation.mutate();

		CHECK_FALSE(ancestor == mutation);
		CHECK(ancestor != mutation);

		mutation.mutate();

		CHECK_FALSE(ancestor == mutation);
		CHECK(ancestor != mutation);
	}
}
