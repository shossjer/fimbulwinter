#include "utility/variant.hpp"

#include "utility/property.hpp"

#include <catch/catch.hpp>

#include <iostream>
#include <string>

namespace
{
	struct counter
	{
		static int destructions;
		static int default_constructions;
		static int copy_constructions;
		static int move_constructions;
		static int copy_assignments;
		static int move_assignments;

		static void reset()
		{
			destructions = 0;
			default_constructions = 0;
			copy_constructions = 0;
			move_constructions = 0;
			copy_assignments = 0;
			move_assignments = 0;
		}

		~counter() { destructions++; }
		counter() { default_constructions++; }
		counter(const counter &) { copy_constructions++; }
		counter(counter &&) { move_constructions++; }
		counter & operator = (const counter &) { copy_assignments++; return *this; }
		counter & operator = (counter &&) { move_assignments++; return *this; }
	};

	int counter::destructions;
	int counter::default_constructions;
	int counter::copy_constructions;
	int counter::move_constructions;
	int counter::copy_assignments;
	int counter::move_assignments;

	struct not_default_constructible :
		utility::enable_default_constructor<false>,
		counter
	{};
	struct not_copy_constructible :
		utility::enable_copy_constructor<false>,
		counter
	{};
	struct not_move_constructible :
		utility::enable_move_constructor<false>,
		counter
	{};
	struct not_copy_assignable :
		utility::enable_copy_assignment<false>,
		counter
	{};
	struct not_move_assignable :
		utility::enable_move_assignment<false>,
		counter
	{};

	struct throws_on_move
	{
		throws_on_move() = default;
		throws_on_move(const throws_on_move &) = default;
		throws_on_move(throws_on_move &&) { throw 0; }
		throws_on_move & operator = (const throws_on_move &) = default;
		throws_on_move & operator = (throws_on_move &&) = default;
	};
}

TEST_CASE( "variant default constructor", "[constructor][default][utility][variant]" )
{
	utility::variant<std::string, int, long> v1;
	REQUIRE(utility::holds_alternative<std::string>(v1));
	////////////////
	using variant2 = utility::variant<not_default_constructible, std::string>;
	// variant2 v2; // should not compile
	static_assert(!std::is_default_constructible<variant2>::value, "");
}

TEST_CASE( "variant copy constructor", "[constructor][copy][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{int(1)};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(1 == utility::get<int>(v1));

	utility::variant<std::string, int, long> v2{v1};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v1));
	REQUIRE(1 == utility::get<int>(v2));
	////////////////
	using variant2 = utility::variant<not_copy_constructible, std::string>;
	variant2 v3;
	// variant2 v4{v3}; // should not compile
	static_assert(!std::is_copy_constructible<variant2>::value, "");
}

TEST_CASE( "variant move constructor", "[constructor][move][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{int(1)};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(1 == utility::get<int>(v1));

	utility::variant<std::string, int, long> v2{std::move(v1)};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v2));
	////////////////
	using variant2 = utility::variant<not_move_constructible, std::string>;
	counter::reset();

	variant2 v3;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	variant2 v4{std::move(v3)};
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(1 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);
}

TEST_CASE( "variant in place constructor", "[constructor][in place][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{utility::in_place_index<0>};
	REQUIRE(utility::holds_alternative<std::string>(v1));

	utility::variant<std::string, int, long> v2{utility::in_place_index<1>};
	REQUIRE(utility::holds_alternative<int>(v2));

	utility::variant<std::string, int, long> v3{utility::in_place_index<2>};
	REQUIRE(utility::holds_alternative<long>(v3));
}

TEST_CASE( "variant conversion constructor", "[constructor][conversion][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{1};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(1 == utility::get<int>(v1));

	utility::variant<std::string, int, long> v2{"a"};
	REQUIRE(utility::holds_alternative<std::string>(v2));
	REQUIRE("a" == utility::get<std::string>(v2));
}

TEST_CASE( "variant copy assignment", "[assignment][copy][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{utility::in_place_type<int>, 1};
	utility::variant<std::string, int, long> v2;
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE_FALSE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v1));

	v2 = v1;
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v1));
	REQUIRE(1 == utility::get<int>(v2));
	////////////////
	using variant2 = utility::variant<not_copy_assignable, std::string>;
	variant2 v3;
	variant2 v4;
	// v4 = v3; // should not compile
	static_assert(!std::is_copy_assignable<variant2>::value, "");
}

TEST_CASE( "variant move assignment", "[assignment][move][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{utility::in_place_type<int>, 1};
	utility::variant<std::string, int, long> v2;
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE_FALSE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v1));

	v2 = std::move(v1);
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(utility::holds_alternative<int>(v2));
	REQUIRE(1 == utility::get<int>(v2));
	////////////////
	using variant2 = utility::variant<not_move_assignable, std::string>;
	counter::reset();

	variant2 v3;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	variant2 v4;
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	v4 = std::move(v3);
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(1 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);
}

TEST_CASE( "variant conversion assignment", "[assignment][conversion][utility][variant]" )
{
	utility::variant<int, std::string, throws_on_move> v1{utility::in_place_type<int>, 1};
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(1 == utility::get<int>(v1));

	v1 = 2;
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(2 == utility::get<int>(v1));

	REQUIRE_THROWS_AS(v1 = throws_on_move{}, int);
	REQUIRE(v1.valueless_by_exception());

	v1 = "a";
	REQUIRE(utility::holds_alternative<std::string>(v1));
	REQUIRE("a" == utility::get<std::string>(v1));
}

TEST_CASE( "variant comparison operators", "[comparison][operator][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{utility::in_place_type<int>, 1};
	utility::variant<std::string, int, long> v2{utility::in_place_type<int>, 2};
	utility::variant<std::string, int, long> v3{utility::in_place_type<std::string>, ""};
	utility::variant<std::string, int, long> v4{utility::in_place_type<long>, 0};

	CHECK_FALSE(v1 == v2);
	CHECK(v1 != v2);
	CHECK(v1 < v2);
	CHECK_FALSE(v1 > v2);
	CHECK(v1 <= v2);
	CHECK_FALSE(v1 >= v2);

	CHECK_FALSE(v1 == v3);
	CHECK(v1 != v3);
	CHECK_FALSE(v1 < v3);
	CHECK(v1 > v3);
	CHECK_FALSE(v1 <= v3);
	CHECK(v1 >= v3);

	CHECK_FALSE(v1 == v4);
	CHECK(v1 != v4);
	CHECK(v1 < v4);
	CHECK_FALSE(v1 > v4);
	CHECK(v1 <= v4);
	CHECK_FALSE(v1 >= v4);
}

TEST_CASE( "variant swap", "[swap][utility][variant]" )
{
	utility::variant<std::string, int, long> v1{utility::in_place_type<std::string>, "a"};
	utility::variant<std::string, int, long> v2{utility::in_place_type<int>, 2};
	REQUIRE(utility::holds_alternative<std::string>(v1));
	REQUIRE(utility::holds_alternative<int>(v2));
	CHECK("a" == utility::get<std::string>(v1));
	CHECK(2 == utility::get<int>(v2));

	swap(v1, v2);
	REQUIRE(utility::holds_alternative<int>(v1));
	REQUIRE(utility::holds_alternative<std::string>(v2));
	CHECK(2 == utility::get<int>(v1));
	CHECK("a" == utility::get<std::string>(v2));
}

namespace
{
	struct setter
	{
		void operator () (std::string & x) { x = "a"; }
		template <typename X>
		void operator () (X &) { throw 1; }
	};
	struct getter
	{
		const std::string & operator () (const std::string & x) { return x; }
		template <typename X>
		const std::string & operator () (const X &) { throw 1; }
	};
}
TEST_CASE( "variant visitation", "[utility][variant]" )
{
	utility::variant<std::string, int, long> v1;
	REQUIRE(utility::holds_alternative<std::string>(v1));
	CHECK("" == utility::get<std::string>(v1));

	visit(setter{}, v1);
	REQUIRE(utility::holds_alternative<std::string>(v1));
	CHECK("a" == utility::get<std::string>(v1));

	const utility::variant<std::string, int, long> & v2 = v1;

	const std::string & x = visit(getter{}, v2);
	CHECK("a" == x);
}
