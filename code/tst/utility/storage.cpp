#include "utility/storage.hpp"

#include <catch2/catch.hpp>

TEST_CASE("static_storage<5, int>", "[utility]")
{
	using static_storage = utility::static_storage<5, int>;
	static_assert(std::is_standard_layout<static_storage>::value, "");
	static_assert(std::is_trivially_destructible<static_storage>::value, "");
	static_assert(static_storage::storing_trivially_copyable::value, "");
	static_assert(static_storage::storing_trivially_destructible::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_allocate::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_deallocate::value, "");

	static_storage aa1;
	// aa1.allocate(5); // trivial to allocate
	aa1.construct_at_(aa1.begin() + 0, 5);
	aa1.construct_at_(aa1.begin() + 1, 4);
	aa1.construct_at_(aa1.begin() + 2, 3);
	aa1.construct_at_(aa1.begin() + 3, 2);
	aa1.construct_at_(aa1.begin() + 4, 1);

	static_assert(std::is_trivially_copy_constructible<static_storage>::value, "");
	static_storage aa2 = aa1;
	CHECK(*aa2.data(aa2.begin() + 0) == 5);
	CHECK(*aa2.data(aa2.begin() + 1) == 4);
	CHECK(*aa2.data(aa2.begin() + 2) == 3);
	CHECK(*aa2.data(aa2.begin() + 3) == 2);
	CHECK(*aa2.data(aa2.begin() + 4) == 1);

	*aa2.data(aa2.begin() + 2) = 6;

	// aa1.destruct_range(aa1.begin(), aa1.end()); // trivially destructible
	// aa1.deallocate(5); // trivial deallocate
	static_assert(std::is_trivially_copy_assignable<static_storage>::value, "");
	aa1 = aa2;
	CHECK(*aa1.data(aa1.begin() + 0) == 5);
	CHECK(*aa1.data(aa1.begin() + 1) == 4);
	CHECK(*aa1.data(aa1.begin() + 2) == 6);
	CHECK(*aa1.data(aa1.begin() + 3) == 2);
	CHECK(*aa1.data(aa1.begin() + 4) == 1);

	static_assert(std::is_trivially_move_constructible<static_storage>::value, "");
	static_storage aa3 = std::move(aa1);
	CHECK(*aa3.data(aa3.begin() + 0) == 5);
	CHECK(*aa3.data(aa3.begin() + 1) == 4);
	CHECK(*aa3.data(aa3.begin() + 2) == 6);
	CHECK(*aa3.data(aa3.begin() + 3) == 2);
	CHECK(*aa3.data(aa3.begin() + 4) == 1);

	*aa3.data(aa3.begin() + 2) = 9;

	// aa1.destruct_range(aa1.begin(), aa1.end()); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_trivially_move_assignable<static_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(*aa1.data(aa1.begin() + 0) == 5);
	CHECK(*aa1.data(aa1.begin() + 1) == 4);
	CHECK(*aa1.data(aa1.begin() + 2) == 9);
	CHECK(*aa1.data(aa1.begin() + 3) == 2);
	CHECK(*aa1.data(aa1.begin() + 4) == 1);

	// aa3.destruct_range(aa3.begin(), aa3.end()); // moved from, nothing to destruct
	// aa2.destruct_range(aa2.begin(), aa2.end()); // trivial to destruct
	// aa1.destruct_range(aa1.begin(), aa1.end()); // trivial to destruct
	// aa3.deallocate(5); // moved from, nothing to deallocate
	// aa2.deallocate(5); // static array has trivial deallocate
	// aa1.deallocate(5); // static array has trivial deallocate
}

TEST_CASE("static_storage<5, unique_ptr>", "[utility]")
{
	using static_storage = utility::static_storage<5, std::unique_ptr<int>>;
	static_assert(std::is_standard_layout<static_storage>::value, "");
	static_assert(!std::is_trivially_destructible<static_storage>::value, "");
	static_assert(!static_storage::storing_trivially_copyable::value, "");
	static_assert(!static_storage::storing_trivially_destructible::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_allocate::value, "");
	static_assert(utility::storage_traits<static_storage>::trivial_deallocate::value, "");

	static_storage aa1;
	// aa1.allocate(5); // static array has trivial allocate
	aa1.construct_at(aa1.begin() + 0, new int(5));
	aa1.construct_at(aa1.begin() + 1, new int(4));
	aa1.construct_at(aa1.begin() + 2, new int(3));
	aa1.construct_at(aa1.begin() + 3, new int(2));
	aa1.construct_at(aa1.begin() + 4, new int(1));

	static_assert(!std::is_copy_constructible<static_storage>::value, "");
	static_assert(!std::is_copy_assignable<static_storage>::value, "");

	static_assert(!std::is_move_constructible<static_storage>::value, "");
	static_assert(!std::is_move_assignable<static_storage>::value, "");

	aa1.destruct_range(aa1.begin(), aa1.end());
	// aa1.deallocate(5); // static array has trivial deallocate
}

TEST_CASE("heap_storage<int>", "[utility]")
{
	using heap_storage = utility::heap_storage<int>;
	static_assert(std::is_standard_layout<heap_storage>::value, "");
#if MODE_DEBUG
	static_assert(!std::is_trivially_destructible<heap_storage>::value, "");
#else
	static_assert(std::is_trivially_destructible<heap_storage>::value, "");
#endif
	static_assert(heap_storage::storing_trivially_copyable::value, "");
	static_assert(heap_storage::storing_trivially_destructible::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_allocate::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_deallocate::value, "");

	heap_storage aa1;
	aa1.allocate(5);
	aa1.construct_at(aa1.begin(5) + 0, 5);
	aa1.construct_at(aa1.begin(5) + 1, 4);
	aa1.construct_at(aa1.begin(5) + 2, 3);
	aa1.construct_at(aa1.begin(5) + 3, 2);
	aa1.construct_at(aa1.begin(5) + 4, 1);

	static_assert(!std::is_copy_constructible<heap_storage>::value, "");
	static_assert(!std::is_copy_assignable<heap_storage>::value, "");

	static_assert(std::is_move_constructible<heap_storage>::value, "");
	heap_storage aa3 = std::move(aa1);
	CHECK(*aa3.data(aa3.begin(5) + 0) == 5);
	CHECK(*aa3.data(aa3.begin(5) + 1) == 4);
	CHECK(*aa3.data(aa3.begin(5) + 2) == 3);
	CHECK(*aa3.data(aa3.begin(5) + 3) == 2);
	CHECK(*aa3.data(aa3.begin(5) + 4) == 1);

	*aa3.data(aa3.begin(5) + 2) = 9;

	// aa1.destruct_range(aa1.begin(5), aa1.end(5)); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_move_assignable<heap_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(*aa1.data(aa1.begin(5) + 0) == 5);
	CHECK(*aa1.data(aa1.begin(5) + 1) == 4);
	CHECK(*aa1.data(aa1.begin(5) + 2) == 9);
	CHECK(*aa1.data(aa1.begin(5) + 3) == 2);
	CHECK(*aa1.data(aa1.begin(5) + 4) == 1);

	// aa3.destruct_range(aa3.begin(5), aa3.end(5)); // moved from, nothing to destruct
	// aa1.destruct_range(aa1.begin(5), aa3.end(5)); // trivial to destruct
	// aa3.deallocate(5); // moved from, nothing to deallocate
	aa1.deallocate(5);
}

TEST_CASE("heap_storage<unique_ptr>", "[utility]")
{
	using heap_storage = utility::heap_storage<std::unique_ptr<int>>;
	static_assert(std::is_standard_layout<heap_storage>::value, "");
#if MODE_DEBUG
	static_assert(!std::is_trivially_destructible<heap_storage>::value, "");
#else
	static_assert(std::is_trivially_destructible<heap_storage>::value, "");
#endif
	static_assert(!heap_storage::storing_trivially_copyable::value, "");
	static_assert(!heap_storage::storing_trivially_destructible::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_allocate::value, "");
	static_assert(!utility::storage_traits<heap_storage>::trivial_deallocate::value, "");

	heap_storage aa1;
	aa1.allocate(5);
	aa1.construct_at(aa1.begin(5) + 0, new int(5));
	aa1.construct_at(aa1.begin(5) + 1, new int(4));
	aa1.construct_at(aa1.begin(5) + 2, new int(3));
	aa1.construct_at(aa1.begin(5) + 3, new int(2));
	aa1.construct_at(aa1.begin(5) + 4, new int(1));

	static_assert(!std::is_copy_constructible<heap_storage>::value, "");
	static_assert(!std::is_copy_assignable<heap_storage>::value, "");

	static_assert(std::is_move_constructible<heap_storage>::value, "");
	heap_storage aa3 = std::move(aa1);
	CHECK(**aa3.data(aa3.begin(5) + 0) == 5);
	CHECK(**aa3.data(aa3.begin(5) + 1) == 4);
	CHECK(**aa3.data(aa3.begin(5) + 2) == 3);
	CHECK(**aa3.data(aa3.begin(5) + 3) == 2);
	CHECK(**aa3.data(aa3.begin(5) + 4) == 1);

	// aa1.destruct_range(aa1.begin(5), aa1.end(5)); // moved from, nothing to destruct
	// aa1.deallocate(5); // moved from, nothing to deallocate
	static_assert(std::is_move_assignable<heap_storage>::value, "");
	aa1 = std::move(aa3);
	CHECK(**aa1.data(aa1.begin(5) + 0) == 5);
	CHECK(**aa1.data(aa1.begin(5) + 1) == 4);
	CHECK(**aa1.data(aa1.begin(5) + 2) == 3);
	CHECK(**aa1.data(aa1.begin(5) + 3) == 2);
	CHECK(**aa1.data(aa1.begin(5) + 4) == 1);

	// aa3.destruct_range(aa3.begin(5), aa3.endi(5)); // moved from, nothing to destruct
	aa1.destruct_range(aa1.begin(5), aa1.end(5));
	// aa3.deallocate(5); // moved from, nothing to deallocate
	aa1.deallocate(5);
}
