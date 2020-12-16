#include "utility/weak_ptr.hpp"

#include <catch2/catch.hpp>

namespace
{
	struct Counter
	{
		static int destruction_counter;
		static int default_construction_counter;
		static int copy_construction_counter;
		static int move_construction_counter;
		static int copy_assignment_counter;
		static int move_assignment_counter;

		~Counter() { destruction_counter++; }
		Counter() { default_construction_counter++; }
		Counter(const Counter &) { copy_construction_counter++; }
		Counter(Counter &&) { move_construction_counter++; }
		Counter & operator = (const Counter &) { copy_assignment_counter++; return *this; }
		Counter & operator = (Counter &&) { move_assignment_counter++; return *this; }

		static void reset()
		{
			destruction_counter = 0;
			default_construction_counter = 0;
			copy_construction_counter = 0;
			move_construction_counter = 0;
			copy_assignment_counter = 0;
			move_assignment_counter = 0;
		}
	};

	int Counter::destruction_counter;
	int Counter::default_construction_counter;
	int Counter::copy_construction_counter;
	int Counter::move_construction_counter;
	int Counter::copy_assignment_counter;
	int Counter::move_assignment_counter;
}

TEST_CASE("weak_ptr", "[utility]")
{
	Counter::reset();

	ext::heap_weak_ptr<Counter> x;
	CHECK(x.expired());
	CHECK(Counter::destruction_counter == 0);
	CHECK(Counter::default_construction_counter == 0);
	CHECK(Counter::copy_construction_counter == 0);
	CHECK(Counter::move_construction_counter == 0);
	CHECK(Counter::copy_assignment_counter == 0);
	CHECK(Counter::move_assignment_counter == 0);

	ext::heap_shared_ptr<Counter> s(utility::in_place);
	REQUIRE(s);

	ext::heap_weak_ptr<Counter> y = s;
	CHECK_FALSE(y.expired());
	CHECK(Counter::destruction_counter == 0);
	CHECK(Counter::default_construction_counter == 1);
	CHECK(Counter::copy_construction_counter == 0);
	CHECK(Counter::move_construction_counter == 0);
	CHECK(Counter::copy_assignment_counter == 0);
	CHECK(Counter::move_assignment_counter == 0);

	SECTION("can be copied")
	{
		ext::heap_weak_ptr<Counter> z = y;
		CHECK_FALSE(z.expired());
		CHECK_FALSE(y.expired());

		z = x;
		CHECK(z.expired());
		CHECK(x.expired());
	}

	SECTION("can be moved")
	{
		ext::heap_weak_ptr<Counter> z = std::move(y);
		CHECK_FALSE(z.expired());
		CHECK(y.expired());

		z = std::move(x);
		CHECK(z.expired());
		CHECK(x.expired());
	}

	SECTION("can be copied/moved to itself")
	{
		y = y;
		CHECK_FALSE(y.expired());

		y = std::move(y);
		// note after move, y is formally only 'valid but unspecified' so the following states should not be relied on elsewhere
		CHECK(y.expired());
	}

	SECTION("does not destruct the contained object")
	{
		y.reset();
		CHECK(Counter::destruction_counter == 0);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);

		s.reset();
		CHECK(Counter::destruction_counter == 1);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);
	}

	SECTION("")
	{
		{
			ext::heap_shared_ptr<Counter> t = y.lock();
		}
		CHECK(Counter::destruction_counter == 0);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);

		s.reset();
		CHECK(Counter::destruction_counter == 1);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);

		CHECK(y.expired());
	}
}
