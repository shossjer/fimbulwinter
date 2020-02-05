#include "utility/aggregation_allocator.hpp"
#include "utility/heap_allocator.hpp"
#include "utility/null_allocator.hpp"

#include <catch/catch.hpp>

TEST_CASE("aggregation allocator (null, void, char, short, int) has default constructor", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::null_allocator, void, char, short, int> aa;
	static_cast<void>(aa);
}

TEST_CASE("aggregation allocator (null, void, char, short, int) fails to allocate", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::null_allocator, void, char, short, int> aa;

	CHECK_FALSE(aa.allocate(0));
}

TEST_CASE("aggregation allocator (null, char, char, short, int) has default constructor", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::null_allocator, char, char, short, int> aa;
	static_cast<void>(aa);
}

TEST_CASE("aggregation allocator (null, char, char, short, int) fails to allocate", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::null_allocator, char, char, short, int> aa;

	CHECK_FALSE(aa.allocate(0));
}

TEST_CASE("aggregation allocator (heap, void, char, short, int) can allocate zero capacity", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::heap_allocator, void, char, short, int> aa;

	void * const p = aa.allocate(0);
	REQUIRE(p);

	aa.deallocate(p, 0);
}

TEST_CASE("aggregation allocator (heap, void, char, short, int) can allocate non-zero capacity", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::heap_allocator, void, char, short, int> aa;

	void * const p = aa.allocate(1);
	REQUIRE(p);

	CHECK(static_cast<void *>(aa.address<0>(p, 1)) != static_cast<void *>(aa.address<1>(p, 1)));
	CHECK(static_cast<void *>(aa.address<0>(p, 1)) != static_cast<void *>(aa.address<2>(p, 1)));

	aa.construct(aa.address<0>(p, 1), '0');
	aa.construct(aa.address<1>(p, 1), short(1));
	aa.construct(aa.address<2>(p, 1), 2);
	CHECK(aa.address<0>(p, 1)[0] == '0');
	CHECK(aa.address<1>(p, 1)[0] == 1);
	CHECK(aa.address<2>(p, 1)[0] == 2);

	aa.destroy(aa.address<2>(p, 1));
	aa.destroy(aa.address<1>(p, 1));
	aa.destroy(aa.address<0>(p, 1));
	aa.deallocate(p, 1);
}

TEST_CASE("aggregation allocator (heap, char, char, short, int) can allocate zero capacity", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::heap_allocator, char, char, short, int> aa;

	char * const p = aa.allocate(0);
	REQUIRE(p);

	aa.construct(p, 'a');
	CHECK(*p == 'a');

	aa.destroy(p);
	aa.deallocate(p, 0);
}

TEST_CASE("aggregation allocator (heap, char, char, short, int) can allocate non-zero capacity", "[allocator][utility]")
{
	utility::aggregation_allocator<utility::heap_allocator, char, char, short, int> aa;

	char * const p = aa.allocate(1);
	REQUIRE(p);

	CHECK(static_cast<void *>(aa.address<0>(p, 1)) != static_cast<void *>(aa.address<1>(p, 1)));
	CHECK(static_cast<void *>(aa.address<0>(p, 1)) != static_cast<void *>(aa.address<2>(p, 1)));

	aa.construct(p, 'a');
	aa.construct(aa.address<0>(p, 1), '0');
	aa.construct(aa.address<1>(p, 1), short(1));
	aa.construct(aa.address<2>(p, 1), 2);
	CHECK(*p == 'a');
	CHECK(aa.address<0>(p, 1)[0] == '0');
	CHECK(aa.address<1>(p, 1)[0] == 1);
	CHECK(aa.address<2>(p, 1)[0] == 2);

	aa.destroy(aa.address<2>(p, 1));
	aa.destroy(aa.address<1>(p, 1));
	aa.destroy(aa.address<0>(p, 1));
	aa.destroy(p);
	aa.deallocate(p, 1);
}
