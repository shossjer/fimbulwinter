
#include "catch.hpp"

#include "utility/array_alloc.hpp"

TEST_CASE( "static_storage<int, 5>", "[utility]" )
{
	using static_storage = utility::static_storage<int, 5>;
	static_assert(std::is_standard_layout<static_storage>::value, "");
	static_assert(std::is_trivially_destructible<static_storage>::value, "");
	static_assert(static_storage::storing_trivially_copyable::value, "");
	static_assert(static_storage::storing_trivially_destructible::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_allocate::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_deallocate::value, "");

	static_storage aa1;
	// aa1.allocate(5); // trivial to allocate
	aa1.construct_at(0, 5);
	aa1.construct_at(1, 4);
	aa1.construct_at(2, 3);
	aa1.construct_at(3, 2);
	aa1.construct_at(4, 1);

	static_assert(std::is_trivially_copy_constructible<static_storage>::value, "");
	static_storage aa2 = aa1;
	CHECK(aa2.data()[0] == 5);
	CHECK(aa2.data()[1] == 4);
	CHECK(aa2.data()[2] == 3);
	CHECK(aa2.data()[3] == 2);
	CHECK(aa2.data()[4] == 1);

	aa2.data()[2] = 6;

	// aa1.destruct_range(0, 5); // trivially destructible
	// aa1.deallocate(5); // trivial deallocate
	static_assert(std::is_trivially_copy_assignable<static_storage>::value, "");
	aa1 = aa2;
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 6);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	static_assert(std::is_trivially_move_constructible<static_storage>::value, "");
	static_storage aa3 = std::move(aa1);
	CHECK(aa3.data()[0] == 5);
	CHECK(aa3.data()[1] == 4);
	CHECK(aa3.data()[2] == 6);
	CHECK(aa3.data()[3] == 2);
	CHECK(aa3.data()[4] == 1);

	aa3.data()[2] = 9;

	// aa1.destruct_range(0, 5); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_trivially_move_assignable<static_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 9);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // moved from, nothing to destruct
	// aa2.destruct_range(0, 5); // trivial to destruct
	// aa1.destruct_range(0, 5); // trivial to destruct
	// aa3.deallocate(5); // moved from, nothing to deallocate
	// aa2.deallocate(5); // static array has trivial deallocate
	// aa1.deallocate(5); // static array has trivial deallocate
}

TEST_CASE( "static_storage<unique_ptr, 5>", "[utility]" )
{
	using static_storage = utility::static_storage<std::unique_ptr<int>, 5>;
	static_assert(std::is_standard_layout<static_storage>::value, "");
	static_assert(!std::is_trivially_destructible<static_storage>::value, "");
	static_assert(!static_storage::storing_trivially_copyable::value, "");
	static_assert(!static_storage::storing_trivially_destructible::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_allocate::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_deallocate::value, "");

	static_storage aa1;
	// aa1.allocate(5); // static array has trivial allocate
	aa1.construct_at(0, new int(5));
	aa1.construct_at(1, new int(4));
	aa1.construct_at(2, new int(3));
	aa1.construct_at(3, new int(2));
	aa1.construct_at(4, new int(1));

	static_assert(!std::is_copy_constructible<static_storage>::value, "");
	static_assert(!std::is_copy_assignable<static_storage>::value, "");

	static_assert(!std::is_move_constructible<static_storage>::value, "");
	static_assert(!std::is_move_assignable<static_storage>::value, "");

	aa1.destruct_range(0, 5);
	// aa1.deallocate(5); // static array has trivial deallocate
}

TEST_CASE( "heap_storage<int, dynamic_alloc>", "[utility]" )
{
	using heap_storage = utility::heap_storage<int>;
	static_assert(std::is_standard_layout<heap_storage>::value, "");
	static_assert(!std::is_trivially_destructible<heap_storage>::value, "");
	static_assert(heap_storage::storing_trivially_copyable::value, "");
	static_assert(heap_storage::storing_trivially_destructible::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_allocate::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_deallocate::value, "");

	heap_storage aa1;
	aa1.allocate(5);
	aa1.construct_at(0, 5);
	aa1.construct_at(1, 4);
	aa1.construct_at(2, 3);
	aa1.construct_at(3, 2);
	aa1.construct_at(4, 1);

	static_assert(!std::is_copy_constructible<heap_storage>::value, "");
	static_assert(!std::is_copy_assignable<heap_storage>::value, "");

	static_assert(std::is_move_constructible<heap_storage>::value, "");
	heap_storage aa3 = std::move(aa1);
	CHECK(aa3.data()[0] == 5);
	CHECK(aa3.data()[1] == 4);
	CHECK(aa3.data()[2] == 3);
	CHECK(aa3.data()[3] == 2);
	CHECK(aa3.data()[4] == 1);

	aa3.data()[2] = 9;

	// aa1.destruct_range(0, 5); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_move_assignable<heap_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(aa1.data()[0] == 5);
	CHECK(aa1.data()[1] == 4);
	CHECK(aa1.data()[2] == 9);
	CHECK(aa1.data()[3] == 2);
	CHECK(aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // moved from, nothing to destruct
	// aa1.destruct_range(0, 5); // trivial to destruct
	// aa3.deallocate(5); // moved from, nothing to deallocate
	aa1.deallocate(5);
}

TEST_CASE( "heap_storage<unique_ptr, dynamic_alloc>", "[utility]" )
{
	using heap_storage = utility::heap_storage<std::unique_ptr<int>>;
	static_assert(std::is_standard_layout<heap_storage>::value, "");
	static_assert(!std::is_trivially_destructible<heap_storage>::value, "");
	static_assert(!heap_storage::storing_trivially_copyable::value, "");
	static_assert(!heap_storage::storing_trivially_destructible::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_allocate::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_deallocate::value, "");

	heap_storage aa1;
	aa1.allocate(5);
	aa1.construct_at(0, new int(5));
	aa1.construct_at(1, new int(4));
	aa1.construct_at(2, new int(3));
	aa1.construct_at(3, new int(2));
	aa1.construct_at(4, new int(1));

	static_assert(!std::is_copy_constructible<heap_storage>::value, "");
	static_assert(!std::is_copy_assignable<heap_storage>::value, "");

	static_assert(std::is_move_constructible<heap_storage>::value, "");
	heap_storage aa3 = std::move(aa1);
	CHECK(*aa3.data()[0] == 5);
	CHECK(*aa3.data()[1] == 4);
	CHECK(*aa3.data()[2] == 3);
	CHECK(*aa3.data()[3] == 2);
	CHECK(*aa3.data()[4] == 1);

	// aa1.destruct_range(0, 5); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_move_assignable<heap_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(*aa1.data()[0] == 5);
	CHECK(*aa1.data()[1] == 4);
	CHECK(*aa1.data()[2] == 3);
	CHECK(*aa1.data()[3] == 2);
	CHECK(*aa1.data()[4] == 1);

	// aa3.destruct_range(0, 5); // moved from, nothing to destruct
	aa1.destruct_range(0, 5);
	// aa3.deallocate(5); // moved from, nothing to deallocate
	aa1.deallocate(5);
}

TEST_CASE( "array_alloc", "[utility]" )
{
	using array_alloc1 = utility::array_alloc<utility::static_storage_traits<5>, int>;
	static_assert(std::is_trivially_destructible<array_alloc1>::value, "");
	static_assert(std::is_trivially_default_constructible<array_alloc1>::value, "");
	static_assert(std::is_trivially_copy_constructible<array_alloc1>::value, "");
	static_assert(std::is_trivially_move_constructible<array_alloc1>::value, "");
	static_assert(std::is_trivially_copy_assignable<array_alloc1>::value, "");
	static_assert(std::is_trivially_move_assignable<array_alloc1>::value, "");
	static_assert(std::is_trivial<array_alloc1>::value, "");

	using array_alloc2 = utility::array_alloc<utility::static_storage_traits<5>, std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<array_alloc2>::value, "");
	static_assert(!std::is_trivially_default_constructible<array_alloc2>::value, "");
	static_assert(std::is_default_constructible<array_alloc2>::value, "");
	static_assert(!std::is_copy_constructible<array_alloc2>::value, "");
	static_assert(!std::is_move_constructible<array_alloc2>::value, "");
	static_assert(!std::is_copy_assignable<array_alloc2>::value, "");
	static_assert(!std::is_move_assignable<array_alloc2>::value, "");
	static_assert(!std::is_trivial<array_alloc2>::value, "");

	using array_alloc3 = utility::array_alloc<utility::heap_storage_traits, int>;
	static_assert(!std::is_trivially_destructible<array_alloc3>::value, "");
	static_assert(!std::is_trivially_default_constructible<array_alloc3>::value, "");
	static_assert(std::is_default_constructible<array_alloc3>::value, "");
	static_assert(!std::is_trivially_copy_constructible<array_alloc3>::value, "");
	static_assert(std::is_copy_constructible<array_alloc3>::value, "");
	static_assert(!std::is_trivially_move_constructible<array_alloc3>::value, "");
	static_assert(std::is_move_constructible<array_alloc3>::value, "");
	static_assert(!std::is_trivially_copy_assignable<array_alloc3>::value, "");
	static_assert(std::is_copy_assignable<array_alloc3>::value, "");
	static_assert(!std::is_trivially_move_assignable<array_alloc3>::value, "");
	static_assert(std::is_move_assignable<array_alloc3>::value, "");
	static_assert(!std::is_trivial<array_alloc3>::value, "");

	using array_alloc4 = utility::array_alloc<utility::heap_storage_traits, std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<array_alloc4>::value, "");
	static_assert(!std::is_trivially_default_constructible<array_alloc4>::value, "");
	static_assert(std::is_default_constructible<array_alloc4>::value, "");
	static_assert(!std::is_copy_constructible<array_alloc4>::value, "");
	static_assert(!std::is_trivially_move_constructible<array_alloc4>::value, "");
	static_assert(std::is_move_constructible<array_alloc4>::value, "");
	static_assert(!std::is_copy_assignable<array_alloc4>::value, "");
	static_assert(!std::is_trivially_move_assignable<array_alloc4>::value, "");
	static_assert(std::is_move_assignable<array_alloc4>::value, "");
	static_assert(!std::is_trivial<array_alloc4>::value, "");
}
