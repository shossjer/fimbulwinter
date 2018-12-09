
#include "catch.hpp"

#include "utility/array_alloc.hpp"

#include <memory>

TEST_CASE( "array_alloc<int, 5>", "[utility]" )
{
	using array_alloc = utility::array_alloc<int, 5>;
	static_assert(std::is_trivially_destructible<array_alloc>::value, "");
	static_assert(array_alloc::value_trivially_destructible, "");

	array_alloc aa1 = {5, 4, 3, 2, 1};
	CHECK(!aa1.empty());

	static_assert(std::is_trivially_copy_constructible<array_alloc>::value, "");
	array_alloc aa2 = aa1;
	CHECK(!aa1.empty());
	CHECK(!aa2.empty());
	CHECK(aa2.data()[0] == 5);
	CHECK(aa2.data()[1] == 4);
	CHECK(aa2.data()[2] == 3);
	CHECK(aa2.data()[3] == 2);
	CHECK(aa2.data()[4] == 1);

	// aa1.destruct_range(0, 5); // trivial to destruct
	aa2.data()[2] = 6;

	static_assert(std::is_trivially_copy_assignable<array_alloc>::value, "");
	aa1 = aa2;
	CHECK(!aa1.empty());
	CHECK(!aa2.empty());
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 6);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	static_assert(std::is_trivially_move_constructible<array_alloc>::value, "");
	array_alloc aa3 = std::move(aa1);
	CHECK(!aa1.empty());
	CHECK(!aa3.empty());
	CHECK(aa3.data()[0] == 5);
	CHECK(aa3.data()[1] == 4);
	CHECK(aa3.data()[2] == 6);
	CHECK(aa3.data()[3] == 2);
	CHECK(aa3.data()[4] == 1);

	// aa1.destruct_range(0, 5); // trivial to destruct, also moved from
	aa3.data()[2] = 9;

	static_assert(std::is_trivially_move_assignable<array_alloc>::value, "");
	aa1 = std::move(aa3);
	CHECK(!aa1.empty());
	CHECK(!aa3.empty());
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 9);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // trivial to destruct, also moved from
	// aa2.destruct_range(0, 5); // trivial to destruct
	// aa1.destruct_range(0, 5); // trivial to destruct
}

TEST_CASE( "array_alloc<unique_ptr, 5>", "[utility]" )
{
	using array_alloc = utility::array_alloc<std::unique_ptr<int>, 5>;
	static_assert(!std::is_trivially_destructible<array_alloc>::value, "");
	static_assert(!array_alloc::value_trivially_destructible, "");

	array_alloc aa1;
	CHECK(!aa1.empty());
	aa1.construct_at(0, new int(5));
	aa1.construct_at(1, new int(4));
	aa1.construct_at(2, new int(3));
	aa1.construct_at(3, new int(2));
	aa1.construct_at(4, new int(1));

	static_assert(!std::is_copy_constructible<array_alloc>::value, "");
	static_assert(!std::is_copy_assignable<array_alloc>::value, "");

	static_assert(!std::is_move_constructible<array_alloc>::value, "");
	static_assert(!std::is_move_assignable<array_alloc>::value, "");

	aa1.destruct_range(0, 5);
}

TEST_CASE( "array_alloc<int, dynamic_alloc>", "[utility]" )
{
	using array_alloc = utility::array_alloc<int, utility::dynamic_alloc>;
	static_assert(!std::is_trivially_destructible<array_alloc>::value, "");
	static_assert(array_alloc::value_trivially_destructible, "");

	array_alloc aa1(5);
	CHECK(!aa1.empty());
	aa1.construct_at(0, 5);
	aa1.construct_at(1, 4);
	aa1.construct_at(2, 3);
	aa1.construct_at(3, 2);
	aa1.construct_at(4, 1);

	static_assert(!std::is_copy_constructible<array_alloc>::value, "");
	static_assert(!std::is_copy_assignable<array_alloc>::value, "");

	static_assert(std::is_move_constructible<array_alloc>::value, "");
	array_alloc aa3 = std::move(aa1);
	CHECK(aa1.empty());
	CHECK(!aa3.empty());
	CHECK(aa3.data()[0] == 5);
	CHECK(aa3.data()[1] == 4);
	CHECK(aa3.data()[2] == 3);
	CHECK(aa3.data()[3] == 2);
	CHECK(aa3.data()[4] == 1);

	// aa1.destruct_range(0, 5); // trivial to destruct, also moved from
	aa3.data()[2] = 9;

	static_assert(std::is_move_assignable<array_alloc>::value, "");
	aa1 = std::move(aa3);
	CHECK(!aa1.empty());
	CHECK(aa3.empty());
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 9);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // trivial to destruct, also moved from
	// aa1.destruct_range(0, 5); // trivial to destruct
}

TEST_CASE( "array_alloc<unique_ptr, dynamic_alloc>", "[utility]" )
{
	using array_alloc = utility::array_alloc<std::unique_ptr<int>, utility::dynamic_alloc>;
	static_assert(!std::is_trivially_destructible<array_alloc>::value, "");
	static_assert(!array_alloc::value_trivially_destructible, "");

	array_alloc aa1(5);
	CHECK(!aa1.empty());
	aa1.construct_at(0, new int(5));
	aa1.construct_at(1, new int(4));
	aa1.construct_at(2, new int(3));
	aa1.construct_at(3, new int(2));
	aa1.construct_at(4, new int(1));

	static_assert(!std::is_copy_constructible<array_alloc>::value, "");
	static_assert(!std::is_copy_assignable<array_alloc>::value, "");

	static_assert(std::is_move_constructible<array_alloc>::value, "");
	array_alloc aa3 = std::move(aa1);
	CHECK(aa1.empty());
	CHECK(!aa3.empty());
	CHECK(*aa3.data()[0] == 5);
	CHECK(*aa3.data()[1] == 4);
	CHECK(*aa3.data()[2] == 3);
	CHECK(*aa3.data()[3] == 2);
	CHECK(*aa3.data()[4] == 1);

	static_assert(std::is_move_assignable<array_alloc>::value, "");
	CHECK(aa1.empty());
	// in order for the move assignment to be correct you have to
	// guarantee that there are no active elements in `aa1` (the one
	// moved to) because the destructor on those elements will not be
	// called
	aa1 = std::move(aa3);
	CHECK(!aa1.empty());
	CHECK(aa3.empty());
	CHECK(*aa1.data()[0] == 5);
	CHECK(*aa1.data()[1] == 4);
	CHECK(*aa1.data()[2] == 3);
	CHECK(*aa1.data()[3] == 2);
	CHECK(*aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // moved from, nothing to destruct
	aa1.destruct_range(0, 5);
}
