#include "utility/property.hpp"

#include <catch/catch.hpp>

#include <type_traits>

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
}

TEST_CASE( "disable default constructor", "[constructor][default][utility][property]" )
{
	static_assert(std::is_destructible<not_default_constructible>::value, "");
	static_assert(!std::is_default_constructible<not_default_constructible>::value, "");
	static_assert(std::is_copy_constructible<not_default_constructible>::value, "");
	static_assert(std::is_move_constructible<not_default_constructible>::value, "");
	static_assert(std::is_copy_assignable<not_default_constructible>::value, "");
	static_assert(std::is_move_assignable<not_default_constructible>::value, "");
}

TEST_CASE( "disable copy constructor", "[constructor][copy][utility][property]" )
{
	static_assert(std::is_destructible<not_copy_constructible>::value, "");
	static_assert(std::is_default_constructible<not_copy_constructible>::value, "");
	static_assert(!std::is_copy_constructible<not_copy_constructible>::value, "");
	static_assert(std::is_move_constructible<not_copy_constructible>::value, "");
	static_assert(std::is_copy_assignable<not_copy_constructible>::value, "");
	static_assert(std::is_move_assignable<not_copy_constructible>::value, "");

	counter::reset();

	not_copy_constructible a;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	not_copy_constructible b(std::move(a));
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(1 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);
}

TEST_CASE( "disable move constructor", "[constructor][move][utility][property]" )
{
	static_assert(std::is_destructible<not_move_constructible>::value, "");
	static_assert(std::is_default_constructible<not_move_constructible>::value, "");
	static_assert(std::is_copy_constructible<not_move_constructible>::value, "");
	static_assert(std::is_move_constructible<not_move_constructible>::value, ""); // since it defaults to copy constructor
	static_assert(std::is_copy_assignable<not_move_constructible>::value, "");
	static_assert(std::is_move_assignable<not_move_constructible>::value, "");

	counter::reset();

	not_move_constructible a;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	not_move_constructible b(std::move(a));
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(1 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);
}

TEST_CASE( "disable copy assignment", "[assignment][copy][utility][property]" )
{
	static_assert(std::is_destructible<not_copy_assignable>::value, "");
	static_assert(std::is_default_constructible<not_copy_assignable>::value, "");
	static_assert(std::is_copy_constructible<not_copy_assignable>::value, "");
	static_assert(std::is_move_constructible<not_copy_assignable>::value, "");
	static_assert(!std::is_copy_assignable<not_copy_assignable>::value, "");
	static_assert(std::is_move_assignable<not_copy_assignable>::value, "");

	counter::reset();

	not_copy_assignable a;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	not_copy_assignable b;
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	b = std::move(a);
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(1 == counter::move_assignments);
}

TEST_CASE( "disable move assignment", "[assignment][move][utility][property]" )
{
	static_assert(std::is_destructible<not_move_assignable>::value, "");
	static_assert(std::is_default_constructible<not_move_assignable>::value, "");
	static_assert(std::is_copy_constructible<not_move_assignable>::value, "");
	static_assert(std::is_move_constructible<not_move_assignable>::value, "");
	static_assert(std::is_copy_assignable<not_move_assignable>::value, "");
	static_assert(std::is_move_assignable<not_move_assignable>::value, ""); // since it defaults to copy assignment

	counter::reset();

	not_move_assignable a;
	CHECK(0 == counter::destructions);
	CHECK(1 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	not_move_assignable b;
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(0 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);

	b = std::move(a);
	CHECK(0 == counter::destructions);
	CHECK(2 == counter::default_constructions);
	CHECK(0 == counter::copy_constructions);
	CHECK(0 == counter::move_constructions);
	CHECK(1 == counter::copy_assignments);
	CHECK(0 == counter::move_assignments);
}
