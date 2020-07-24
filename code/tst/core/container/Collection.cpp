#include "core/container/Collection.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct get_value
	{
		int operator () (utility::monostate) { return -1; }
		int operator () (int x) { return x; }
		int operator () (long long x) { return (int)x; }
		template <typename X>
		int operator () (X &&) { throw -1; }
	};
	struct set_value
	{
		void operator () (utility::monostate) {}
		void operator () (int & x) { x = 5; }
		void operator () (long long & x) { x = 5ll; }
		template <typename X>
		int operator () (X &&) { throw -1; }
	};
}

TEST_CASE("multicollection", "[core][container]")
{
	core::container::MultiCollection
	<
		unsigned,
		100,
		std::array<float, 10>,
		std::array<int, 10>,
		std::array<long long, 10>,
		std::array<char, 10>
	> collection;

	CHECK(!collection.contains(7u));
	CHECK(!collection.contains<float>(7u));
	CHECK(!collection.contains<int>(7u));
	CHECK(!collection.contains<long long>(7u));
	CHECK(!collection.contains<char>(7u));

	collection.emplace<int>(7u, 2);
	collection.emplace<long long>(7u, 3ll);
	REQUIRE(collection.contains(7u));
	CHECK(!collection.contains<float>(7u));
	CHECK(!collection.contains<char>(7u));
	REQUIRE(collection.contains<int>(7u));
	REQUIRE(collection.contains<long long>(7u));
	CHECK(collection.get<int>(7u) == 2);
	CHECK(collection.get<long long>(7u) == 3ll);

	collection.emplace<int>(5u, 5);
	collection.emplace<char>(5u, 'g');
	CHECK(collection.contains(7u));
	REQUIRE(collection.contains(5u));
	CHECK(!collection.contains<float>(5u));
	CHECK(!collection.contains<long long>(5u));
	REQUIRE(collection.contains<int>(5u));
	REQUIRE(collection.contains<char>(5u));
	CHECK(collection.get<int>(5u) == 5);
	CHECK(collection.get<char>(5u) == 'g');

	for (auto && x : collection.get<int>())
	{
		const auto key = collection.get_key(x);
		const auto kjhs = key == 5u || key == 7u;
		CHECK(kjhs);
	}

	{
		const int val_int = collection.call<int>(7u, get_value{});
		const int val_long_long = collection.call<long long>(7u, get_value{});
		CHECK(val_int == 2);
		CHECK(val_long_long == 3);
		CHECK_THROWS(collection.call<char>(5u, get_value{}));
	}
	{
		const int val_int = collection.call(7u, get_value{});
		CHECK(val_int == 2);
	}
	{
		collection.call_all(7u, set_value{});
	}
	{
		const int val_int = collection.try_call<int>(7u, get_value{});
		const int val_long_long = collection.try_call<long long>(7u, get_value{});
		const int val_invalid_id = collection.try_call<int>(3u, get_value{});
		const int val_invalid_type = collection.try_call<char>(7u, get_value{});
		CHECK(val_int == 5);
		CHECK(val_long_long == 5);
		CHECK(val_invalid_id == -1);
		CHECK(val_invalid_type == -1);
	}
	{
		const int val_int = collection.try_call(7u, get_value{});
		const int val_invalid_id = collection.try_call(3u, get_value{});
		CHECK(val_int == 5);
		CHECK(val_invalid_id == -1);
	}
	{
		collection.try_call_all(7u, set_value{});
		collection.try_call_all(3u, set_value{});
	}

	collection.remove<int>(7u);
	CHECK(collection.contains(7u));
	CHECK(!collection.contains<int>(7u));
	CHECK(!collection.contains<float>(7u));
	CHECK(!collection.contains<char>(7u));
	CHECK(collection.contains<long long>(7u));

	collection.remove(5u);
	CHECK(collection.contains(7u));
	CHECK(!collection.contains(5u));
	CHECK(!collection.contains<float>(5u));
	CHECK(!collection.contains<int>(5u));
	CHECK(!collection.contains<long long>(5u));
	CHECK(!collection.contains<char>(5u));
}

TEST_CASE("collection heap_storage", "[core][container]")
{
	core::container::Collection
	<
		unsigned int,
		utility::heap_storage_traits,
		utility::heap_storage<int>
	>
	collection;

	CHECK(collection.emplace<int>(1u, 1));
	CHECK(collection.emplace<int>(2u, 2));
	CHECK(collection.emplace<int>(3u, 3));
	CHECK(collection.emplace<int>(4u, 4));
	CHECK(collection.emplace<int>(5u, 5));

	SECTION("can get the key for values")
	{
		auto ints = collection.get<int>();
		REQUIRE(ints.size() == 5);

		auto it = ints.begin();
		CHECK(*it == 1);
		CHECK(collection.get_key(*it) == 1u);
		++it;
		CHECK(*it == 2);
		CHECK(collection.get_key(*it) == 2u);
		++it;
		CHECK(*it == 3);
		CHECK(collection.get_key(*it) == 3u);
		++it;
		CHECK(*it == 4);
		CHECK(collection.get_key(*it) == 4u);
		++it;
		CHECK(*it == 5);
		CHECK(collection.get_key(*it) == 5u);
		++it;
		CHECK(it == ints.end());
	}
}

TEST_CASE("collection static_storage", "[core][container]")
{
	core::container::Collection
	<
		unsigned int,
		utility::static_storage_traits<10>,
		utility::static_storage<5, int>
	>
	collection;

	CHECK(collection.emplace<int>(1u, 1));
	CHECK(collection.emplace<int>(2u, 2));
	CHECK(collection.emplace<int>(3u, 3));
	CHECK(collection.emplace<int>(4u, 4));
	CHECK(collection.emplace<int>(5u, 5));

	SECTION("fails to emplace when full")
	{
		CHECK(!collection.emplace<int>(6u, 6));
	}

	SECTION("can get the key for values")
	{
		auto ints = collection.get<int>();
		REQUIRE(ints.size() == 5);

		auto it = ints.begin();
		CHECK(*it == 1);
		CHECK(collection.get_key(*it) == 1u);
		++it;
		CHECK(*it == 2);
		CHECK(collection.get_key(*it) == 2u);
		++it;
		CHECK(*it == 3);
		CHECK(collection.get_key(*it) == 3u);
		++it;
		CHECK(*it == 4);
		CHECK(collection.get_key(*it) == 4u);
		++it;
		CHECK(*it == 5);
		CHECK(collection.get_key(*it) == 5u);
		++it;
		CHECK(it == ints.end());
	}
}
