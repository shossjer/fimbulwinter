#include "utility/container/array.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct construction_counter
	{
		static ext::usize construction_count;
		static ext::usize destruction_count;

		static void reset()
		{
			construction_count = 0;
			destruction_count = 0;
		}

		ext::usize i;

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
	};

	ext::usize construction_counter::construction_count = ext::usize(-1);
	ext::usize construction_counter::destruction_count = ext::usize(-1);
}

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

TEST_CASE("trivial static array", "[utility][container][array]")
{
	utility::static_array<10, int, double, char> a;
	CHECK(a.capacity() == 10);
	CHECK(a.size() == a.capacity());

	SECTION("can be copied")
	{
		REQUIRE(a.reserve(1));

		utility::static_array<10, int, double, char> b = a;
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = b;
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can be moved")
	{
		REQUIRE(a.reserve(1));

		utility::static_array<10, int, double, char> b = std::move(a);
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = std::move(b);
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can reserve space")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == a.capacity());

		SECTION("but not too much")
		{
			CHECK_FALSE(a.reserve(20));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == a.capacity());
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == a.capacity());
		}
	}
}

TEST_CASE("trivial heap array", "[utility][container][array]")
{
	utility::heap_array<int, double, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	SECTION("can be copied")
	{
		REQUIRE(a.reserve(1));

		utility::heap_array<int, double, char> b = a;
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = b;
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can be moved")
	{
		REQUIRE(a.reserve(1));

		utility::heap_array<int, double, char> b = std::move(a);
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() == 0);
		CHECK(a.size() == a.capacity());

		a = std::move(b);
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() == 0);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can reserve space")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		CHECK(a.size() == a.capacity());

		SECTION("and increase it a second time")
		{
			CHECK(a.reserve(20));
			CHECK(a.capacity() >= 20);
			CHECK(a.size() == a.capacity());
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.reserve(5));
			CHECK(a.capacity() >= 10);
			CHECK(a.size() == a.capacity());
		}
	}
}

TEST_CASE("static array", "[utility][container][array]")
{
	construction_counter::reset();

	utility::static_array<10, int, construction_counter, char> a;
	CHECK(a.capacity() == 10);
	REQUIRE(a.size() == a.capacity());
	CHECK(construction_counter::construction_count == 10);
	CHECK(construction_counter::destruction_count == 0);
	CHECK(std::get<1>(a.data())[0].i == 1);
	CHECK(std::get<1>(a.data())[9].i == 10);

	SECTION("can be copied")
	{
		REQUIRE(a.reserve(1));

		utility::static_array<10, int, construction_counter, char> b = a;
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = b;
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can be moved")
	{
		REQUIRE(a.reserve(1));

		utility::static_array<10, int, construction_counter, char> b = std::move(a);
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = std::move(b);
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can reserve space")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == 10);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[9].i == 10);

		SECTION("but not too much")
		{
			CHECK_FALSE(a.reserve(20));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == 10);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[9].i == 10);
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == 10);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[9].i == 10);
		}

		SECTION("but not reset")
		{
			REQUIRE_FALSE(a.reset());
			CHECK(a.capacity() == 10);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == 10);
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[9].i == 10);
		}
	}

	SECTION("can resize space, but only to its static size")
	{
		CHECK_FALSE(a.resize(9));
		CHECK_FALSE(a.resize(11));

		CHECK(a.resize(10));
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == 10);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[9].i == 10);
	}

	SECTION("can be iterated")
	{
		int i = 0;
		for (auto && elem : a)
		{
			i++;
			CHECK(std::get<1>(elem).i == i);
		}
		CHECK(i == 10);
	}
}

TEST_CASE("heap array", "[utility][container][array]")
{
	construction_counter::reset();

	utility::heap_array<int, construction_counter, char> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	SECTION("can be copied")
	{
		REQUIRE(a.reserve(1));

		utility::heap_array<int, construction_counter, char> b = a;
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());

		a = b;
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can be moved")
	{
		REQUIRE(a.reserve(1));

		utility::heap_array<int, construction_counter, char> b = std::move(a);
		CHECK(b.capacity() >= 1);
		CHECK(b.size() == b.capacity());
		CHECK(a.capacity() == 0);
		CHECK(a.size() == a.capacity());

		a = std::move(b);
		CHECK(a.capacity() >= 1);
		CHECK(a.size() == a.capacity());
		CHECK(b.capacity() == 0);
		CHECK(b.size() == b.capacity());
	}

	SECTION("can reserve space")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == a.size());
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[a.size() - 1].i == a.size());

		SECTION("and increase it a second time")
		{
			const auto old_size = a.size();

			CHECK(a.reserve(old_size + 1));
			CHECK(a.capacity() >= old_size + 1);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == old_size + a.size());
			CHECK(construction_counter::destruction_count == old_size);
			CHECK(std::get<1>(a.data())[0].i == old_size + 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == old_size + a.size());
		}

		SECTION("and reserving less than what we already have does nothing")
		{
			CHECK(a.reserve(5));
			CHECK(a.capacity() >= 10);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == a.size());
			CHECK(construction_counter::destruction_count == 0);
			CHECK(std::get<1>(a.data())[0].i == 1);
			CHECK(std::get<1>(a.data())[a.size() - 1].i == a.size());
		}

		SECTION("and reset")
		{
			const auto old_size = a.size();

			REQUIRE(a.reset());
			CHECK(a.capacity() == 0);
			REQUIRE(a.size() == a.capacity());
			CHECK(construction_counter::construction_count == old_size);
			CHECK(construction_counter::destruction_count == old_size);
		}
	}

	SECTION("can resize space")
	{
		CHECK(a.resize(5));
		CHECK(a.capacity() == 5);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == 5);
		CHECK(construction_counter::destruction_count == 0);
		CHECK(std::get<1>(a.data())[0].i == 1);
		CHECK(std::get<1>(a.data())[4].i == 5);

		CHECK(a.resize(10));
		CHECK(a.capacity() == 10);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == 15);
		CHECK(construction_counter::destruction_count == 5);
		CHECK(std::get<1>(a.data())[0].i == 6);
		CHECK(std::get<1>(a.data())[9].i == 15);

		CHECK(a.resize(5));
		CHECK(a.capacity() == 5);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::construction_count == 20);
		CHECK(construction_counter::destruction_count == 15);
		CHECK(std::get<1>(a.data())[0].i == 16);
		CHECK(std::get<1>(a.data())[4].i == 20);
	}

	SECTION("can be iterated")
	{
		int i = 0;
		for (auto && elem : a)
		{
			i++;
			CHECK(std::get<1>(elem).i == i);
		}
		CHECK(i == 0);

		REQUIRE(a.reserve(7));

		for (auto && elem : a)
		{
			i++;
			CHECK(std::get<1>(elem).i == i);
		}
		CHECK(i == 7);
	}
}

TEST_CASE("trivial static array understands zero initialization", "[utility][container][array]")
{
	utility::array<utility::static_storage<10, int, double, char>, utility::initialize_zero> a;
	REQUIRE(a.size() == 10);
	for (ext::index i = 0; static_cast<std::size_t>(i) < a.size(); i++)
	{
		CHECK(std::get<0>(a.data())[i] == 0);
		CHECK(std::get<1>(a.data())[i] == 0.);
		CHECK(std::get<2>(a.data())[i] == 0);
	}
}

TEST_CASE("trivial heap array understands zero initialization", "[utility][container][array]")
{
	utility::array<utility::heap_storage<int, double, char>, utility::initialize_zero> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	SECTION("")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == a.capacity());
		for (ext::index i = 0; static_cast<std::size_t>(i) < a.size(); i++)
		{
			CHECK(std::get<0>(a.data())[i] == 0);
			CHECK(std::get<1>(a.data())[i] == 0.);
			CHECK(std::get<2>(a.data())[i] == 0);
		}
	}
}

TEST_CASE("static array understands zero initialization", "[utility][container][array]")
{
	construction_counter::reset();

	utility::array<utility::static_storage<10, int, construction_counter, char>, utility::initialize_zero> a;
	REQUIRE(a.size() == 10);
	for (ext::usize i = 0; i < a.size(); i++)
	{
		CHECK(std::get<0>(a.data())[i] == 0);
		CHECK(std::get<1>(a.data())[i].i == i + 1);
		CHECK(std::get<2>(a.data())[i] == 0);
	}
}

TEST_CASE("heap array understands zero initialization", "[utility][container][array]")
{
	construction_counter::reset();

	utility::array<utility::heap_storage<int, construction_counter, char>, utility::initialize_zero> a;
	CHECK(a.capacity() == 0);
	CHECK(a.size() == a.capacity());

	SECTION("")
	{
		CHECK(a.reserve(10));
		CHECK(a.capacity() >= 10);
		REQUIRE(a.size() == a.capacity());
		CHECK(construction_counter::destruction_count == 0);
		for (ext::usize i = 0; i < a.size(); i++)
		{
			CHECK(std::get<0>(a.data())[i] == 0);
			CHECK(std::get<1>(a.data())[i].i == i + 1);
			CHECK(std::get<2>(a.data())[i] == 0);
		}
	}
}
