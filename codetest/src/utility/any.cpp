
#include "catch.hpp"

#include <utility/any.hpp>

#include <string>

TEST_CASE( "any default constructor", "[constructor][default][utility][any]" )
{
	utility::any a1;
	CHECK(!a1.has_value());
}

TEST_CASE( "any copy constructor", "[constructor][copy][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = a1;
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = a1;
		REQUIRE(a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 1);
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any move constructor", "[constructor][move][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = std::move(a1);
		CHECK(!a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any conversion constructor", "[constructor][conversion][utility][any]" )
{
	utility::any a1 = int{1};
	REQUIRE(a1.has_value());
	REQUIRE(utility::any_cast<int>(a1) == 1);
}

TEST_CASE( "any in_place constructor", "[constructor][in_place][utility][any]" )
{
	utility::any a1{utility::in_place_type<int>, 1};
	REQUIRE(a1.has_value());
	REQUIRE(utility::any_cast<int>(a1) == 1);
}

TEST_CASE( "any copy assignment", "[assignment][copy][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		a2 = a1;
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		a2 = a1;
		REQUIRE(a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 1);
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		a2 = a1;
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
}

TEST_CASE( "any move assignment", "[assignment][move][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		a2 = std::move(a1);
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
}

TEST_CASE( "any conversion assignment", "[assignment][conversion][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		a1 = int{1};
		REQUIRE(a1.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 1);
	}
	SECTION( "" )
	{
		utility::any a1 = int{-1};
		a1 = int{1};
		REQUIRE(a1.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 1);
	}
}

TEST_CASE( "any swap", "[swap][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		CHECK(!a2.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2;
		swap(a1, a2);
		CHECK(!a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
	SECTION( "" )
	{
		utility::any a1;
		utility::any a2 = int{2};
		swap(a1, a2);
		REQUIRE(a1.has_value());
		CHECK(!a2.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 2);
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		utility::any a2 = int{2};
		swap(a1, a2);
		REQUIRE(a1.has_value());
		REQUIRE(a2.has_value());
		REQUIRE(utility::any_cast<int>(a1) == 2);
		REQUIRE(utility::any_cast<int>(a2) == 1);
	}
}

TEST_CASE( "any emplace", "[emplace][utility][any]" )
{
	utility::any a1;
	int & i1 = a1.emplace<int>(1);
	REQUIRE(a1.has_value());
	REQUIRE(utility::any_cast<int>(a1) == 1);
	i1 = 2;
	REQUIRE(utility::any_cast<int>(a1) == 2);
	a1.emplace<int>(3);
	REQUIRE(a1.has_value());
	REQUIRE(utility::any_cast<int>(a1) == 3);
}

TEST_CASE( "any reset", "[reset][utility][any]" )
{
	SECTION( "" )
	{
		utility::any a1;
		a1.reset();
		CHECK(!a1.has_value());
		a1.reset();
		CHECK(!a1.has_value());
	}
	SECTION( "" )
	{
		utility::any a1 = int{1};
		a1.reset();
		CHECK(!a1.has_value());
		a1.reset();
		CHECK(!a1.has_value());
	}
}

TEST_CASE( "any has_value", "[has_value][utility][any]" )
{
}

TEST_CASE( "any any_cast", "[any_cast][utility][any]" )
{
}
