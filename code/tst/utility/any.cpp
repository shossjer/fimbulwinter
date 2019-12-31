
#include "catch.hpp"

#include "utility/any.hpp"

#include <string>

TEST_CASE( "any default constructor", "[utility][any]" )
{
	utility::any a1;
	CHECK(!a1.has_value());
	CHECK(a1.type_id() == utility::type_id<void>());
}

TEST_CASE( "any copy constructor", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = a1;
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = a1;
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
		CHECK(a2.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any move constructor", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any conversion constructor", "[utility][any]" )
{
	utility::any a1 = int{1};
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 1);
}

TEST_CASE( "any in_place constructor", "[utility][any]" )
{
	utility::any a1{utility::in_place_type<int>, 1};
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 1);
}

TEST_CASE( "any copy assignment", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		a2 = a1;
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		a2 = a1;
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		a2 = a1;
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
}

TEST_CASE( "any move assignment", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
}

TEST_CASE( "any conversion assignment", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		a1 = int{1};
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
	}
	SECTION( "" )
	{
		utility::any a1 = int{-1};
		a1 = int{1};
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 1);
	}
}

TEST_CASE( "any swap", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		swap(a1, a2);
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 2);
		CHECK(!a2.has_value());
		CHECK(a2.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = int{2};
		swap(a1, a2);
		CHECK(a1.has_value());
		REQUIRE(a1.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a1) == 2);
		CHECK(a2.has_value());
		REQUIRE(a2.type_id() == utility::type_id<int>());
		CHECK(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any emplace", "[utility][any]" )
{
	utility::any a1;
	int & i1 = a1.emplace<int>(1);
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 1);
	i1 = 2;
	CHECK(utility::any_cast<int>(a1) == 2);
	a1.emplace<int>(3);
	CHECK(a1.has_value());
	REQUIRE(a1.type_id() == utility::type_id<int>());
	CHECK(utility::any_cast<int>(a1) == 3);
}

TEST_CASE( "any reset", "[utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
		a1.reset();
		CHECK(!a1.has_value());
		CHECK(a1.type_id() == utility::type_id<void>());
	}
}
