#include "utility/container/vector.hpp"

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
	using static_int_vector = utility::static_vector<5, int>;
	static_assert(std::is_trivially_destructible<static_int_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_int_vector>::value, "");
	static_assert(std::is_default_constructible<static_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_int_vector>::value, "");
	static_assert(std::is_copy_constructible<static_int_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_int_vector>::value, "");
	static_assert(std::is_move_constructible<static_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_int_vector>::value, "");
	static_assert(std::is_copy_assignable<static_int_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_int_vector>::value, "");
	static_assert(std::is_move_assignable<static_int_vector>::value, "");

	using static_unique_ptr_vector = utility::static_vector<5, std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<static_unique_ptr_vector>::value, "");
	static_assert(std::is_destructible<static_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_unique_ptr_vector>::value, "");
	static_assert(std::is_default_constructible<static_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_unique_ptr_vector>::value, "");
	static_assert(std::is_copy_constructible<static_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_unique_ptr_vector>::value, "");
	static_assert(std::is_move_constructible<static_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_unique_ptr_vector>::value, "");
	static_assert(std::is_copy_assignable<static_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_unique_ptr_vector>::value, "");
	static_assert(std::is_move_assignable<static_unique_ptr_vector>::value, "");

	using static_int_unique_ptr_int_vector = utility::static_vector<5, int, std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_destructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_default_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_copy_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_move_constructible<static_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_copy_assignable<static_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<static_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_move_assignable<static_int_unique_ptr_int_vector>::value, "");

	using heap_int_vector = utility::heap_vector<int>;
	static_assert(!std::is_trivially_destructible<heap_int_vector>::value, "");
	static_assert(std::is_destructible<heap_int_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_vector>::value, "");
	static_assert(std::is_default_constructible<heap_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_vector>::value, "");
	static_assert(std::is_copy_constructible<heap_int_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_vector>::value, "");
	static_assert(std::is_move_constructible<heap_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_vector>::value, "");
	static_assert(std::is_copy_assignable<heap_int_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_vector>::value, "");
	static_assert(std::is_move_assignable<heap_int_vector>::value, "");

	using heap_unique_ptr_vector = utility::heap_vector<std::unique_ptr<int>>;
	static_assert(!std::is_trivially_destructible<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_destructible<heap_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_default_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_copy_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_move_constructible<heap_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_copy_assignable<heap_unique_ptr_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_unique_ptr_vector>::value, "");
	static_assert(std::is_move_assignable<heap_unique_ptr_vector>::value, "");

	using heap_int_unique_ptr_int_vector = utility::heap_vector<int ,std::unique_ptr<int>, int>;
	static_assert(!std::is_trivially_destructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_destructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_default_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_default_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_copy_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_move_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_move_constructible<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_copy_assignable<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_copy_assignable<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(!std::is_trivially_move_assignable<heap_int_unique_ptr_int_vector>::value, "");
	static_assert(std::is_move_assignable<heap_int_unique_ptr_int_vector>::value, "");
}

TEST_CASE("trivial static vector", "[utility][container][vector]")
{
	utility::static_vector<10, int, double, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 0);

	CHECK(a.try_emplace_back(1, 2., '3'));
	CHECK(a.capacity() == 10);
	REQUIRE(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]) == 2.);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be copy constructed")
	{
		utility::static_vector<10, int, double, char> b = a;
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be copy assigned")
	{
		utility::static_vector<10, int, double, char> b;
		b = a;
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be move constructed")
	{
		utility::static_vector<10, int, double, char> b = std::move(a);
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be move assigned")
	{
		utility::static_vector<10, int, double, char> b;
		b = std::move(a);
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() == 10);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be filled")
	{
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.try_emplace_back(1, 2., '3'));
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 10);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(std::get<0>(a[9]) == 1);
		CHECK(std::get<1>(a[9]) == 2.);
		CHECK(std::get<2>(a[9]) == '3');

		SECTION("and will fail when adding more")
		{
			CHECK_FALSE(a.try_emplace_back(1, 2., '3'));
			CHECK(a.capacity() == 10);
			REQUIRE(a.size() == 10);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			CHECK(std::get<0>(a[9]) == 1);
			CHECK(std::get<1>(a[9]) == 2.);
			CHECK(std::get<2>(a[9]) == '3');
		}

		SECTION("and cleared")
		{
			a.clear();
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 0);
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == 10);
			REQUIRE(a.size() == 9);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			CHECK(std::get<0>(a[8]) == 1);
			CHECK(std::get<1>(a[8]) == 2.);
			CHECK(std::get<2>(a[8]) == '3');
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');

		SECTION("but not too much")
		{
			CHECK_FALSE(a.try_reserve(20));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}
	}
}

TEST_CASE("trivial heap vector", "[utility][container][vector]")
{
	utility::heap_vector<int, double, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == 0);

	CHECK(a.try_emplace_back(1, 2., '3'));
	CHECK(a.capacity() >= 1);
	REQUIRE(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]) == 2.);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be copy constructed")
	{
		utility::heap_vector<int, double, char> b = a;
		CHECK(a.capacity() >= 1);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be copy assigned")
	{
		utility::heap_vector<int, double, char> b;
		b = a;
		CHECK(a.capacity() >= 1);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be move constructed")
	{
		utility::heap_vector<int, double, char> b = std::move(a);
		CHECK(a.capacity() == 0);
		CHECK(a.size() == 0);
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be move assigned")
	{
		utility::heap_vector<int, double, char> b;
		b = std::move(a);
		CHECK(a.capacity() == 0);
		CHECK(a.size() == 0);
		CHECK(b.capacity() >= 1);
		REQUIRE(b.size() == 1);
		CHECK(std::get<0>(b[0]) == 1);
		CHECK(std::get<1>(b[0]) == 2.);
		CHECK(std::get<2>(b[0]) == '3');
	}

	SECTION("can be filled")
	{
		const auto old_cap = a.capacity();

		for (ext::index i = a.size(); static_cast<std::size_t>(i) < old_cap; i++)
		{
			CHECK(a.try_emplace_back(1, 2., '3'));
		}
		CHECK(a.capacity() == old_cap);
		REQUIRE(a.size() == old_cap);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');
		CHECK(std::get<0>(a[old_cap - 1]) == 1);
		CHECK(std::get<1>(a[old_cap - 1]) == 2.);
		CHECK(std::get<2>(a[old_cap - 1]) == '3');

		SECTION("and will reallocate when adding more")
		{
			CHECK(a.try_emplace_back(1, 2., '3'));
			CHECK(a.capacity() > old_cap);
			REQUIRE(a.size() == old_cap + 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			CHECK(std::get<0>(a[old_cap]) == 1);
			CHECK(std::get<1>(a[old_cap]) == 2.);
			CHECK(std::get<2>(a[old_cap]) == '3');
		}

		SECTION("and cleared")
		{
			a.clear();
			CHECK(a.capacity() == old_cap);
			CHECK(a.size() == 0);
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == old_cap);
			REQUIRE(a.size() == old_cap - 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
			if (a.size() >= 2)
			{
				CHECK(std::get<0>(a[old_cap - 2]) == 1);
				CHECK(std::get<1>(a[old_cap - 2]) == 2.);
				CHECK(std::get<2>(a[old_cap - 2]) == '3');
			}
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == 1);
		CHECK(std::get<0>(a[0]) == 1);
		CHECK(std::get<1>(a[0]) == 2.);
		CHECK(std::get<2>(a[0]) == '3');

		SECTION("and increase it a second time")
		{
			CHECK(a.try_reserve(20));
			CHECK(a.capacity() >= 20);
			REQUIRE(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(std::get<0>(a[0]) == 1);
			CHECK(std::get<1>(a[0]) == 2.);
			CHECK(std::get<2>(a[0]) == '3');
		}
	}
}

TEST_CASE("static vector", "[utility][container][vector]")
{
	construction_counter::reset();

	utility::static_vector<10, int, construction_counter, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == 0);

	CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
	CHECK(a.capacity() == 10);
	REQUIRE(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]).i == 1);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == 10);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[a.size() - 1].i == 10);

		SECTION("and will fail when adding more")
		{
			CHECK_FALSE(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
			CHECK(a.capacity() == 10);
			REQUIRE(a.size() == 10);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == 10);
		}

		SECTION("and cleared")
		{
			a.clear();
			CHECK(a.capacity() == 10);
			CHECK(a.size() == 0);
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == 10);
			REQUIRE(a.size() == 9);
			CHECK(std::get<1>(a[0]).i == 10);
			CHECK(std::get<1>(a[8]).i == 9);
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == 1);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);

		SECTION("but not too much")
		{
			CHECK_FALSE(a.try_reserve(20));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
		}
	}
}

TEST_CASE("heap vector", "[utility][container][vector]")
{
	construction_counter::reset();

	utility::heap_vector<int, construction_counter, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
	CHECK(a.capacity() >= 1);
	REQUIRE(a.size() == 1);
	CHECK(std::get<0>(a[0]) == 1);
	CHECK(std::get<1>(a[0]).i == 1);
	CHECK(std::get<2>(a[0]) == '3');

	SECTION("can be filled")
	{
		const auto old_cap = a.capacity();

		for (ext::index i = a.size(); static_cast<std::size_t>(i) < old_cap; i++)
		{
			CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
		}
		CHECK(a.capacity() == old_cap);
		REQUIRE(a.size() == old_cap);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[a.size() - 1].i == old_cap);

		SECTION("and will reallocate when adding more")
		{
			CHECK(a.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(1), std::tuple<>(), std::forward_as_tuple('3')));
			CHECK(a.capacity() > old_cap);
			REQUIRE(a.size() == old_cap + 1);
			CHECK(construction_counter::destruction_count == old_cap);
			CHECK(std::get<1>(a.data())[0].i == old_cap + 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == old_cap + a.size());
		}

		SECTION("and cleared")
		{
			a.clear();
			CHECK(a.capacity() == old_cap);
			CHECK(a.size() == 0);
		}

		SECTION("and erase its first element")
		{
			CHECK(a.try_erase(0));
			CHECK(a.capacity() == old_cap);
			REQUIRE(a.size() == old_cap - 1);
			CHECK(std::get<1>(a[0]).i == old_cap);
			if (a.size() >= 2)
			{
				CHECK(std::get<1>(a[old_cap - 2]).i == old_cap - 1);
			}
		}
	}

	SECTION("can reserve space")
	{
		CHECK(a.try_reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == 1);
		CHECK(construction_counter::destruction_count == 1);
		CHECK(std::get<1>(a.data())[0].i == 2);

		SECTION("and increase it a second time")
		{
			const auto old_cap = a.capacity();

			CHECK(a.try_reserve(old_cap + 1));
			CHECK(a.capacity() >= old_cap + 1);
			REQUIRE(a.size() == 1);
			CHECK(construction_counter::destruction_count == 2);
			CHECK(std::get<1>(a.data())[0].i == 3);
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.try_reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == 1);
			CHECK(construction_counter::destruction_count == 1);
			CHECK(std::get<1>(a.data())[0].i == 2);
		}
	}
}
