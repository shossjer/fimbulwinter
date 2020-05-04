#include "utility/container/array.hpp"

#include <catch/catch.hpp>

TEST_CASE("", "")
{
	using static_int_array = utility::static_array<5, int>;
	static_assert(std::is_trivially_destructible<static_int_array>::value, "");
	static_assert(std::is_trivially_default_constructible<static_int_array>::value, "");
	static_assert(std::is_trivially_copy_constructible<static_int_array>::value, "");
	static_assert(std::is_trivially_move_constructible<static_int_array>::value, "");
	static_assert(std::is_trivially_copy_assignable<static_int_array>::value, "");
	static_assert(std::is_trivially_move_assignable<static_int_array>::value, "");

	using static_unique_ptr_array = utility::static_array<5, std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<static_unique_ptr_array>::value, "");
	static_assert(std::is_destructible<static_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_unique_ptr_array>::value, "");
	static_assert(std::is_default_constructible<static_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_unique_ptr_array>::value, "");
	static_assert(std::is_copy_constructible<static_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_unique_ptr_array>::value, "");
	static_assert(std::is_move_constructible<static_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_unique_ptr_array>::value, "");
	static_assert(std::is_copy_assignable<static_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_unique_ptr_array>::value, "");
	static_assert(std::is_move_assignable<static_unique_ptr_array>::value, "");

	using static_int_unique_ptr_int_array = utility::static_array<5, int, std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_destructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_default_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_copy_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_move_constructible<static_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_copy_assignable<static_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_move_assignable<static_int_unique_ptr_int_array>::value, "");

	using heap_int_array = utility::heap_array<int>;
	static_assert(!std::is_trivially_destructible<heap_int_array>::value, "");
	static_assert(std::is_destructible<heap_int_array>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_array>::value, "");
	static_assert(std::is_default_constructible<heap_int_array>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_array>::value, "");
	static_assert(std::is_copy_constructible<heap_int_array>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_array>::value, "");
	static_assert(std::is_move_constructible<heap_int_array>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_array>::value, "");
	static_assert(std::is_copy_assignable<heap_int_array>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_array>::value, "");
	static_assert(std::is_move_assignable<heap_int_array>::value, "");

	using heap_unique_ptr_array = utility::heap_array<std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<heap_unique_ptr_array>::value, "");
	static_assert(std::is_destructible<heap_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_unique_ptr_array>::value, "");
	static_assert(std::is_default_constructible<heap_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_unique_ptr_array>::value, "");
	static_assert(std::is_copy_constructible<heap_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_unique_ptr_array>::value, "");
	static_assert(std::is_move_constructible<heap_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_unique_ptr_array>::value, "");
	static_assert(std::is_copy_assignable<heap_unique_ptr_array>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_unique_ptr_array>::value, "");
	static_assert(std::is_move_assignable<heap_unique_ptr_array>::value, "");

	using heap_int_unique_ptr_int_array = utility::heap_array<int ,std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_destructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_default_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_copy_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_move_constructible<heap_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_copy_assignable<heap_int_unique_ptr_int_array>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_unique_ptr_int_array>::value, "");
	static_assert(std::is_move_assignable<heap_int_unique_ptr_int_array>::value, "");
}
