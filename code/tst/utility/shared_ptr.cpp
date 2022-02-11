#include "utility/shared_ptr.hpp"

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

TEST_CASE("shared_ptr", "[utility]")
{
	Counter::reset();

	ext::heap_shared_ptr<Counter> x;
	CHECK_FALSE(x);
	CHECK(Counter::destruction_counter == 0);
	CHECK(Counter::default_construction_counter == 0);
	CHECK(Counter::copy_construction_counter == 0);
	CHECK(Counter::move_construction_counter == 0);
	CHECK(Counter::copy_assignment_counter == 0);
	CHECK(Counter::move_assignment_counter == 0);

	ext::heap_shared_ptr<Counter> y(utility::in_place);
	CHECK(y);
	CHECK(Counter::destruction_counter == 0);
	CHECK(Counter::default_construction_counter == 1);
	CHECK(Counter::copy_construction_counter == 0);
	CHECK(Counter::move_construction_counter == 0);
	CHECK(Counter::copy_assignment_counter == 0);
	CHECK(Counter::move_assignment_counter == 0);

	SECTION("can be copied")
	{
		{
			ext::heap_shared_ptr<Counter> z = y;
			CHECK(z);
			CHECK(y);

			z = x;
			CHECK_FALSE(z);
			CHECK_FALSE(x);
		}
		CHECK(Counter::destruction_counter == 0);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);

		y.reset();
		CHECK_FALSE(y);
		CHECK(Counter::destruction_counter == 1);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);
	}

	SECTION("can be moved")
	{
		{
			ext::heap_shared_ptr<Counter> z = std::move(y);
			CHECK(z);
			CHECK_FALSE(y);

			z = std::move(x);
			CHECK_FALSE(z);
			CHECK_FALSE(x);
		}
		CHECK(Counter::destruction_counter == 1);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);
	}

	SECTION("can be copied/moved to itself")
	{
		auto & yref = y;
		y = yref;
		CHECK(y);
		CHECK(Counter::destruction_counter == 0);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);

		y = std::move(yref);
		// note after move, y is formally only 'valid but unspecified' so the following states should not be relied on elsewhere
		CHECK_FALSE(y);
		CHECK(Counter::destruction_counter == 1);
		CHECK(Counter::default_construction_counter == 1);
		CHECK(Counter::copy_construction_counter == 0);
		CHECK(Counter::move_construction_counter == 0);
		CHECK(Counter::copy_assignment_counter == 0);
		CHECK(Counter::move_assignment_counter == 0);
	}
}
