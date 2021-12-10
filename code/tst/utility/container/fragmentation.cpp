#include "utility/container/fragmentation.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct construction_counter
	{
		static int construction_count;
		static int destruction_count;

		static void reset()
		{
			construction_count = 0;
			destruction_count = 0;
		}

		int i;

		~construction_counter()
		{
			++destruction_count;
		}
		construction_counter()
			: i(++construction_count)
		{}
		construction_counter(const construction_counter &)
			: i(++construction_count)
		{}
		construction_counter(construction_counter &&)
			: i(++construction_count)
		{}
		construction_counter & operator = (const construction_counter &) = default;
		construction_counter & operator = (construction_counter &&) = default;
	};

	int construction_counter::construction_count = -1;
	int construction_counter::destruction_count = -1;
}

TEST_CASE("", "")
{
	using static_int_fragmentation = utility::static_fragmentation<5, int>;
	static_assert(std::is_trivially_destructible<static_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_int_fragmentation>::value, "");
	static_assert(std::is_default_constructible<static_int_fragmentation>::value, "");
	static_assert(std::is_trivially_copy_constructible<static_int_fragmentation>::value, "");
	static_assert(std::is_trivially_move_constructible<static_int_fragmentation>::value, "");
	static_assert(std::is_trivially_copy_assignable<static_int_fragmentation>::value, "");
	static_assert(std::is_trivially_move_assignable<static_int_fragmentation>::value, "");

	using static_unique_ptr_fragmentation = utility::static_fragmentation<5, std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_destructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_default_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_copy_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_move_constructible<static_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_copy_assignable<static_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_move_assignable<static_unique_ptr_fragmentation>::value, "");

	using static_int_unique_ptr_int_fragmentation = utility::static_fragmentation<5, int, std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_destructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_default_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_copy_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_move_constructible<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_copy_assignable<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_move_assignable<static_int_unique_ptr_int_fragmentation>::value, "");

	using heap_int_fragmentation = utility::heap_fragmentation<int>;
	static_assert(!std::is_trivially_destructible<heap_int_fragmentation>::value, "");
	static_assert(std::is_destructible<heap_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_fragmentation>::value, "");
	static_assert(std::is_default_constructible<heap_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_fragmentation>::value, "");
	static_assert(std::is_copy_constructible<heap_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_fragmentation>::value, "");
	static_assert(std::is_move_constructible<heap_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_fragmentation>::value, "");
	static_assert(std::is_copy_assignable<heap_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_fragmentation>::value, "");
	static_assert(std::is_move_assignable<heap_int_fragmentation>::value, "");

	using heap_unique_ptr_fragmentation = utility::heap_fragmentation<std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_destructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_default_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_copy_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_move_constructible<heap_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_copy_assignable<heap_unique_ptr_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_unique_ptr_fragmentation>::value, "");
	static_assert(std::is_move_assignable<heap_unique_ptr_fragmentation>::value, "");

	using heap_int_unique_ptr_int_fragmentation = utility::heap_fragmentation<int ,std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_destructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_default_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_copy_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_move_constructible<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_copy_assignable<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_unique_ptr_int_fragmentation>::value, "");
	static_assert(std::is_move_assignable<heap_int_unique_ptr_int_fragmentation>::value, "");
}

TEST_CASE("trivial static fragmentation", "[utility][container][fragmentation]")
{
	utility::static_fragmentation<10, int, double, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 0);

	auto index_of = [&a](auto ptr){ return ptr - a.data(); };

	REQUIRE(index_of(a.try_emplace(1, 2., '3')) == 0);
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]) == 2.);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		REQUIRE(index_of(a.try_emplace(1, 2., '3')) == 1);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 2);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 3);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 4);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 5);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 6);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 7);
		CHECK(index_of(a.try_emplace(1, 2., '3')) == 8);
		REQUIRE(index_of(a.try_emplace(1, 2., '3')) == 9);
		CHECK(a.capacity() == 10);
		CHECK(a.size() == 10);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(std::get<0>(a[9]) == 1);
		CHECK(std::get<1>(a[9]) == 2.);
		CHECK(std::get<2>(a[9]) == '3');

		SECTION("and will fail when adding more")
		{
			CHECK_FALSE(a.try_emplace(1, 2., '3'));
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 10);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			CHECK(std::get<0>(a[9]) == 1);
			CHECK(std::get<1>(a[9]) == 2.);
			CHECK(std::get<2>(a[9]) == '3');
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 9);
			CHECK(std::get<0>(a[1]) == 1);
			CHECK(std::get<1>(a[1]) == 2.);
			CHECK(std::get<2>(a[1]) == '3');
			CHECK(std::get<0>(a[9]) == 1);
			CHECK(std::get<1>(a[9]) == 2.);
			CHECK(std::get<2>(a[9]) == '3');
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');

		SECTION("but not too much")
		{
			CHECK_FALSE(a.try_reserve(20));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}
	}
}

TEST_CASE("trivial heap fragmentation", "[utility][container][fragmentation]")
{
	utility::heap_fragmentation<int, double, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == 0);

	auto index_of = [&a](auto ptr){ return ptr - a.data(); };

	REQUIRE(index_of(a.try_emplace(1, 2., '3')) == 0);
	CHECK(a.capacity() >= 1);
	CHECK(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]) == 2.);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		const auto old_cap = a.capacity();

		for (ext::index i = a.size(); static_cast<std::size_t>(i) < old_cap; i++)
		{
			REQUIRE(index_of(a.try_emplace(1, 2., '3')) == i);
		}
		CHECK(a.capacity() == old_cap);
		CHECK(a.size() == old_cap);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(std::get<0>(a[old_cap - 1]) == 1);
		CHECK(std::get<1>(a[old_cap - 1]) == 2.);
		CHECK(std::get<2>(a[old_cap - 1]) == '3');

		SECTION("and will reallocate when adding more")
		{
			REQUIRE(index_of(a.try_emplace(1, 2., '3')) == static_cast<ext::index>(old_cap));
			CHECK(a.capacity() > old_cap);
			CHECK(a.size() == old_cap + 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			CHECK(std::get<0>(a[old_cap]) == 1);
			CHECK(std::get<1>(a[old_cap]) == 2.);
			CHECK(std::get<2>(a[old_cap]) == '3');
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == old_cap);
			REQUIRE(a.size() == old_cap - 1);
			if (old_cap >= 2)
			{
				CHECK(std::get<0>(a[1]) == 1);
				CHECK(std::get<1>(a[1]) == 2.);
				CHECK(std::get<2>(a[1]) == '3');
				CHECK(std::get<0>(a[old_cap - 1]) == 1);
				CHECK(std::get<1>(a[old_cap - 1]) == 2.);
				CHECK(std::get<2>(a[old_cap - 1]) == '3');
			}
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');

		SECTION("and increase it a second time")
		{
			CHECK(a.try_reserve(20));
			CHECK(a.capacity() >= 20);
			CHECK(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}
	}
}

TEST_CASE("static fragmentation", "[utility][container][fragmentation]")
{
	construction_counter::reset();

	utility::static_fragmentation<10, int, construction_counter, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 0);

	auto index_of = [&a](auto ptr){ return ptr - a.data(); };

	REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 0);
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]).i == 1);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 1);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 2);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 3);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 4);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 5);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 6);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 7);
		CHECK(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 8);
		REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 9);
		CHECK(a.capacity() == 10);
		CHECK(a.size() == 10);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[a.size() - 1].i == 10);

		SECTION("and will fail when adding more")
		{
			CHECK_FALSE(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 10);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == 10);
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 9);
			CHECK(std::get<1>(a[1]).i == 2);
			CHECK(std::get<1>(a[9]).i == 10);
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == 1);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);

		SECTION("but not too much")
		{
			CHECK_FALSE(a.try_reserve(20));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
		}
	}
}

TEST_CASE("heap fragmentation", "[utility][container][fragmentation]")
{
	construction_counter::reset();

	utility::heap_fragmentation<int, construction_counter, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	auto index_of = [&a](auto ptr){ return ptr - a.data(); };

	REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == 0);
	CHECK(a.capacity() >= 1);
	CHECK(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]).i == 1);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		const auto old_cap = a.capacity();

		for (ext::index i = a.size(); static_cast<std::size_t>(i) < old_cap; i++)
		{
			REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == i);
		}
		CHECK(a.capacity() == old_cap);
		CHECK(a.size() == old_cap);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[a.size() - 1].i == old_cap);

		SECTION("and will reallocate when adding more")
		{
			REQUIRE(index_of(a.try_emplace(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3'))) == static_cast<ext::index>(old_cap));
			CHECK(a.capacity() > old_cap);
			CHECK(a.size() == old_cap + 1);
			CHECK(construction_counter::destruction_count == old_cap);
			CHECK(std::get<1>(a.data())[0].i == old_cap + 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == old_cap + a.size());
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == old_cap);
			CHECK(a.size() == old_cap - 1);
			if (old_cap >= 2)
			{
				CHECK(std::get<1>(a[1]).i == old_cap);
				CHECK(std::get<1>(a[old_cap - 1]).i == old_cap - 1);
			}
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == 1);
		CHECK(construction_counter::destruction_count == 1);
		CHECK(std::get<1>(a.data())[0].i == 2);

		SECTION("and increase it a second time")
		{
			const auto old_cap = a.capacity();

			CHECK(a.try_reserve(old_cap + 1));
			CHECK(a.capacity() >= old_cap + 1);
			CHECK(a.size() == 1);
			CHECK(construction_counter::destruction_count == 2);
			CHECK(std::get<1>(a.data())[0].i == 3);
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == 1);
			CHECK(construction_counter::destruction_count == 1);
			CHECK(std::get<1>(a.data())[0].i == 2);
		}
	}
}
