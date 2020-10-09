#include "core/container/Collection.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct get_value
	{
		int operator () (int x) { return x; }
		int operator () (long long x) { return (int)x; }
		template <typename X>
		int operator () (X &&) { throw -1; }
	};
	struct set_value
	{
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
		utility::static_storage_traits<101>,
		utility::static_storage<10, float>,
		utility::static_storage<10, int>,
		utility::static_storage<10, long long>,
		utility::static_storage<10, char>
	> collection;

	SECTION("is empty by default")
	{
		const auto it = find(collection, 7u);
		CHECK(it == collection.end());
	}

	CHECK(collection.emplace<int>(7u, 2));
	CHECK(collection.emplace<long long>(7u, 3ll));
	const auto seven_it = find(collection, 7u);
	REQUIRE(seven_it != collection.end());
	CHECK_FALSE(collection.contains<float>(seven_it));
	CHECK_FALSE(collection.contains<char>(seven_it));
	CHECK(collection.contains<int>(seven_it));
	CHECK(collection.contains<long long>(seven_it));
	CHECK_FALSE(collection.get<float>(seven_it));
	CHECK_FALSE(collection.get<char>(seven_it));
	REQUIRE(collection.get<int>(seven_it));
	CHECK(*collection.get<int>(seven_it) == 2);
	REQUIRE(collection.get<long long>(seven_it));
	CHECK(*collection.get<long long>(seven_it) == 3ll);

	CHECK(collection.emplace<int>(5u, 5));
	CHECK(collection.emplace<char>(5u, 'g'));
	const auto five_it = find(collection, 5u);
	REQUIRE(five_it != collection.end());
	CHECK_FALSE(collection.contains<float>(five_it));
	CHECK_FALSE(collection.contains<long long>(five_it));
	CHECK(collection.contains<int>(five_it));
	CHECK(collection.contains<char>(five_it));
	CHECK_FALSE(collection.get<float>(five_it));
	CHECK_FALSE(collection.get<long long>(five_it));
	REQUIRE(collection.get<int>(five_it));
	CHECK(*collection.get<int>(five_it) == 5);
	REQUIRE(collection.get<char>(five_it));
	CHECK(*collection.get<char>(five_it) == 'g');

	REQUIRE(find(collection, 7u) == seven_it);

	for (auto && x : collection.get<int>())
	{
		const auto key = collection.get_key(x);
		const auto kjhs = key == 5u || key == 7u;
		CHECK(kjhs);
	}

	{
		collection.call(seven_it, set_value{});
	}

	collection.erase<int>(seven_it);
	CHECK(find(collection, 7u) == seven_it);
	CHECK_FALSE(collection.contains<int>(seven_it));
	CHECK_FALSE(collection.contains<float>(seven_it));
	CHECK_FALSE(collection.contains<char>(seven_it));
	CHECK(collection.contains<long long>(seven_it));

	collection.erase(five_it);
	CHECK(find(collection, 7u) == seven_it);
	CHECK(find(collection, 5u) != five_it);
	CHECK_FALSE(collection.contains<float>(five_it));
	CHECK_FALSE(collection.contains<int>(five_it));
	CHECK_FALSE(collection.contains<long long>(five_it));
	CHECK_FALSE(collection.contains<char>(five_it));
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
