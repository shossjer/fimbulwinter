
#include "catch.hpp"

#include "views_test.hpp"

#include <engine/gui/measure.hpp>

SCENARIO("measure active - initial", "[gui][measure]")
{
	GIVEN("a view with 'fixed' size")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };
		REQUIRE(ViewTester::change(view).affects_size());
		REQUIRE(ViewTester::size(view).height.value == 0);
		REQUIRE(ViewTester::size(view).width.value == 0);

		WHEN("view is measured")
		{
			ViewMeasure::measure_active(view);

			THEN("then the size should be applied")
			{
				REQUIRE(ViewTester::size(view).height.value == 10);
				REQUIRE(ViewTester::size(view).width.value == 20);
			}
		}
	}
	GIVEN("a view with 'wrap' size")
	{
		TestView view{ 10, 20 };
		REQUIRE(ViewTester::change(view).affects_size());
		REQUIRE(ViewTester::size(view).height.value == 0);
		REQUIRE(ViewTester::size(view).width.value == 0);

		WHEN("view is measured")
		{
			ViewMeasure::measure_active(view);

			THEN("then the size should be applied")
			{
				REQUIRE(ViewTester::size(view).height.value == 10);
				REQUIRE(ViewTester::size(view).width.value == 20);
			}
		}
	}
	GIVEN("a view with 'parent' size")
	{
		TestView view{ };
		REQUIRE(ViewTester::change(view).affects_size());
		REQUIRE(ViewTester::size(view).height.value == 0);
		REQUIRE(ViewTester::size(view).width.value == 0);

		WHEN("view is measured")
		{
			ViewMeasure::measure_active(view);

			THEN("size remains unchanged, but change is cleared")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 0);
				REQUIRE(ViewTester::size(view).width.value == 0);
			}
		}
	}
	GIVEN("a view with 'percentage' size")
	{
		TestView view{ Size{ { Size::TYPE::PERCENT } ,{ Size::TYPE::PERCENT } } };
		REQUIRE(ViewTester::change(view).affects_size());
		REQUIRE(ViewTester::size(view).height.value == 0);
		REQUIRE(ViewTester::size(view).width.value == 0);

		WHEN("view is measured")
		{
			ViewMeasure::measure_active(view);

			THEN("size remains unchanged, but change is cleared")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 0);
				REQUIRE(ViewTester::size(view).width.value == 0);
			}
		}
	}
}

SCENARIO("measure active - unchanged", "[gui][measure]")
{
	GIVEN("an already 'measured' wrap view")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };
		ViewMeasure::measure_active(view);
		REQUIRE(ViewTester::change(view).affects_size());
		REQUIRE(ViewTester::size(view).height.value == 10);
		REQUIRE(ViewTester::size(view).width.value == 20);

		WHEN("view again is measured")
		{
			REQUIRE(ViewTester::change(view).affects_size());
			ViewMeasure::measure_active(view);

			THEN("it should have 'size change' cleared")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 10);
				REQUIRE(ViewTester::size(view).width.value == 20);
			}
		}
	}
	// others are the same
}

SCENARIO("measure passive", "[gui][measure]")
{
	GIVEN("a view with 'fixed' size - which is 'size' changed")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };
		REQUIRE(ViewTester::change(view).affects_size());

		WHEN("the 'size changed' view is updated")
		{
			ViewMeasure::measure_passive(view, nullptr);
			THEN("the view should remain unchanged")
			{
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 0);
				REQUIRE(ViewTester::size(view).width.value == 0);
			}
		}
	}
	GIVEN("a view with 'fixed' size - which is NOT 'size' changed")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };
		ViewTester::change(view).clear_size();
		REQUIRE(!ViewTester::change(view).affects_size());

		WHEN("the non-'size changed' view is updated")
		{
			ViewMeasure::measure_passive(view, nullptr);
			THEN("the view should remain unchanged - but 'size change' cleared")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 0);
				REQUIRE(ViewTester::size(view).width.value == 0);
			}
		}
	}

	GIVEN("a view with 'percentage' size - initial")
	{
		TestView view{ Size{ { Size::TYPE::PERCENT, .5f } ,{ Size::TYPE::PERCENT, .5f } } };
		ViewMeasure::measure_active(view);
		REQUIRE(ViewTester::change(view).affects_content());
		REQUIRE(ViewTester::size(view).height.value == 0);
		REQUIRE(ViewTester::size(view).width.value == 0);
		TestGroup group{ { {Size::TYPE::FIXED, 20}, {Size::TYPE::FIXED, 100} }, Layout::RELATIVE };
		ViewMeasure::measure_active(group);
		REQUIRE(ViewTester::size(group).height.value == 20);
		REQUIRE(ViewTester::size(group).width.value == 100);

		WHEN("the view is measured")
		{
			ViewMeasure::measure_passive(view, &group);

			THEN("the view is updated")
			{
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 10);	// 50%
				REQUIRE(ViewTester::size(view).width.value == 50);	// 50%
			}
		}
		AND_WHEN("the view is updated again")
		{
			ViewMeasure::measure_passive(view, &group);
			// update it again ("AND_WHEN" reruns the test)
			ViewMeasure::measure_passive(view, &group);

			THEN("nothing happends")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 10);	// 50%
				REQUIRE(ViewTester::size(view).width.value == 50);	// 50%
			}
		}
		AND_WHEN("the percentage changes")
		{
			ViewMeasure::measure_passive(view, &group);
			// update it again ("AND_WHEN" reruns the test)
			ViewTester::size(view).width.meta = 1.f;
			ViewMeasure::measure_passive(view, &group);

			THEN("the view is updated")
			{
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 10);	// 50%
				REQUIRE(ViewTester::size(view).width.value == 100);	// 100%
			}
		}
	}
}

// verify it updates "PARENT", "PERCENTAGE"
// verify it updates change flag
SCENARIO("measure passive forced", "[gui][measure]")
{
	Size parent_size{ { Size::TYPE::FIXED, 20 } ,{ Size::TYPE::FIXED, 10 } };
	parent_size.height.fixed();
	parent_size.width.fixed();

	GIVEN("a view with 'fixed' size")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };

		WHEN("it is updated with 'changed state'")
		{
			REQUIRE(ViewTester::change(view).affects_size());
			ViewMeasure::measure_passive_forced(view, parent_size);

			THEN("it should remain 'changed'")
			{
				REQUIRE(ViewTester::change(view).affects_size());
			}
		}
		WHEN("it is updated without 'changed state'")
		{
			ViewTester::change(view).clear();
			REQUIRE(!ViewTester::change(view).affects_size());
			ViewMeasure::measure_passive_forced(view, parent_size);

			THEN("it should remain 'non-changed'")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
			}
		}
	}
	GIVEN("a view with 'parent' size - initial")
	{
		TestView view{ Size{ { Size::TYPE::PARENT } ,{ Size::TYPE::PARENT } } };

		WHEN("it is updated (initially)")
		{
			REQUIRE(ViewTester::change(view).affects_size());
			ViewMeasure::measure_passive_forced(view, parent_size);

			THEN("it should be updated")
			{
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 20);
				REQUIRE(ViewTester::size(view).width.value == 10);
			}
		}
		AND_WHEN("it is updated (after parent change)")
		{
			ViewTester::change(view).clear();
			REQUIRE(!ViewTester::change(view).affects_size());
			ViewMeasure::measure_passive_forced(view, parent_size);

			THEN("it should be updated") // same as above
			{
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 20);
				REQUIRE(ViewTester::size(view).width.value == 10);
			}
		}
	}

	GIVEN("a view with 'parent' size - unchanged")
	{
		TestView view{ Size{ { Size::TYPE::PARENT } ,{ Size::TYPE::PARENT } } };
		ViewMeasure::measure_passive_forced(view, parent_size);

		WHEN("it is updated without any change")
		{
			ViewTester::change(view).set_resized();
			REQUIRE(ViewTester::change(view).affects_size());

			ViewMeasure::measure_passive_forced(view, parent_size);

			THEN("it should be cleared (even if marked for change)")
			{
				REQUIRE(!ViewTester::change(view).affects_size());
			}
		}
	}
}
