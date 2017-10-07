
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

			THEN("size remains unchanged, and change is needed")
			{
				REQUIRE(ViewTester::change(view).affects_size());
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

			THEN("the view remains unchanged.")
			{
				REQUIRE(ViewTester::change(view).affects_size());
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
		ViewTester::change(view).clear();
		ViewTester::change(view).set_resized();
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
		ViewTester::change(view).clear();
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
		ViewMeasure::measure_active(group, ViewTester::change(view));
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
			ViewTester::change(view).clear();
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
		ViewTester::change(view).clear();

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

SCENARIO("measure changed 'view' in unchanged groups", "[gui][measure]")
{
	TestGroup group1{ { { Size::TYPE::FIXED, 20 },{ Size::TYPE::FIXED, 100 } }, Layout::RELATIVE };
	ViewMeasure::measure_active(group1, change_t{});
	REQUIRE(ViewTester::size(group1).height.value == 20);
	REQUIRE(ViewTester::size(group1).width.value == 100);

	TestGroup group2{ { { Size::TYPE::PARENT },{ Size::TYPE::PARENT } }, Layout::VERTICAL };
	group1.adopt(&group2);
	ViewMeasure::measure_passive_forced(group2, group1.get_size());
	REQUIRE(ViewTester::size(group2).height.value == 20);
	REQUIRE(ViewTester::size(group2).width.value == 100);

	ViewTester::change(group1).clear();
	ViewTester::change(group2).clear();

	GIVEN("a view with 'fixed' size")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 10 } } };
		group2.adopt(&view);

		WHEN("parent group is 'refreshed'")
		{
			ViewTester::refresh(group1);

			THEN("parent views should be marked as changed.")
			{
				REQUIRE(ViewTester::change(group1).any());
				REQUIRE(!ViewTester::change(group1).affects_size());
				REQUIRE(ViewTester::change(group2).any());
				REQUIRE(!ViewTester::change(group2).affects_size());
				REQUIRE(ViewTester::change(view).affects_size());
			}

			ViewTester::refresh_changes(group1, &group1);

			AND_THEN("after finishing refresh, parents should be done")
			{
				REQUIRE(!ViewTester::change(group1).any());
				REQUIRE(!ViewTester::change(group2).any());

				// the view should be unaffected since 'test view' overrides 'clear'
				REQUIRE(ViewTester::change(view).affects_size());

				ViewMeasure::measure_passive(view, &group2);
				REQUIRE(ViewTester::change(view).affects_size());
			}
		}
	}
	GIVEN("a view with 'parent' size")
	{
		TestView view{ Size{ { Size::TYPE::PARENT } ,{ Size::TYPE::PARENT } } };
		group2.adopt(&view);

		WHEN("parent group is 'refreshed'")
		{
			ViewTester::refresh(group1);

			THEN("parent views should be marked as changed.")
			{
				REQUIRE(ViewTester::change(group1).any());
				REQUIRE(!ViewTester::change(group1).affects_size());
				REQUIRE(ViewTester::change(group2).any());
				REQUIRE(!ViewTester::change(group2).affects_size());
				REQUIRE(ViewTester::change(view).any());
				REQUIRE(ViewTester::change(view).affects_size());
			}

			ViewTester::refresh_changes(group1, &group1);

			AND_THEN("after finishing refresh, parents should be done")
			{
				REQUIRE(!ViewTester::change(group1).any());
				REQUIRE(!ViewTester::change(group2).any());

				// the view should be unaffected since 'test view' overrides 'clear'
				REQUIRE(ViewTester::change(view).affects_size());

				ViewMeasure::measure_passive(view, &group2);
				REQUIRE(ViewTester::change(view).affects_size());
			}
		}
	}
}

SCENARIO("prev. measured group, new child (fixed)", "[gui][measure]")
{
	TestGroup group{ { { Size::TYPE::FIXED, 20 },{ Size::TYPE::FIXED, 100 } }, Layout::RELATIVE };
	ViewMeasure::measure_active(group, change_t{});
	ViewTester::change(group).clear();

	GIVEN("a group which is already measured")
	{
		TestView view{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 20 } } };
		group.adopt(&view);

		WHEN("adding a new child view")
		{
			change_t change = ViewTester::refresh(group);

			THEN("the new child view should be updated and parent 'content updated'.")
			{
				REQUIRE(ViewTester::change(group).any());
				REQUIRE(!ViewTester::change(group).affects_size());
				REQUIRE(ViewTester::change(view).any());
				REQUIRE(ViewTester::change(view).affects_size());
				REQUIRE(ViewTester::size(view).height.value == 10);
				REQUIRE(ViewTester::size(view).width.value == 20);
			}
		}
	}
}
SCENARIO("prev. measured group, new child (wrap)", "[gui][measure]")
{
	TestGroup group{ Size{ { Size::TYPE::WRAP } ,{ Size::TYPE::WRAP } }, Layout::VERTICAL };
	group.wrap_children = true;
	ViewMeasure::measure_active(group, change_t{});
	ViewTester::change(group).clear();

	GIVEN("a group which is already measured")
	{
		REQUIRE(ViewTester::size(group).height.value == 0);
		REQUIRE(ViewTester::size(group).width.value == 0);

		TestView view1{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 40 } } };
		TestView view2{ Size{ { Size::TYPE::FIXED, 10 } ,{ Size::TYPE::FIXED, 40 } } };
		group.adopt(&view1);
		group.adopt(&view2);

		WHEN("adding a new child view")
		{
			REQUIRE(ViewTester::change(view1).affects_size());
			REQUIRE(ViewTester::change(view2).affects_size());
			change_t change = ViewTester::refresh(group);

			THEN("the new child view should be updated and parent also.")
			{
				REQUIRE(ViewTester::change(group).any());
				REQUIRE(ViewTester::change(group).affects_size());
				REQUIRE(ViewTester::change(view1).any());
				REQUIRE(ViewTester::change(view1).affects_size());

				REQUIRE(ViewTester::size(group).height.value == 20);
				REQUIRE(ViewTester::size(group).width.value == 40);
				REQUIRE(ViewTester::size(view1).height.value == 10);
				REQUIRE(ViewTester::size(view1).width.value == 40);
				REQUIRE(ViewTester::size(view2).height.value == 10);
				REQUIRE(ViewTester::size(view2).width.value == 40);
			}

			THEN("verify offset of the new views.")
			{
				ViewTester::refresh_changes(group, &group);

				Vector3f offset1 = ViewTester::offset(view1);
				Vector3f offset2 = ViewTester::offset(view2);

				Vector3f::array_type ob1;
				offset1.get_aligned(ob1);
				REQUIRE(ob1[0] == 0);
				REQUIRE(ob1[1] == 0);
				REQUIRE(ob1[2] == 0);

				Vector3f::array_type ob2;
				offset2.get_aligned(ob2);
				REQUIRE(ob2[0] == 0);
				REQUIRE(ob2[1] == 10);
				REQUIRE(ob2[2] == 0);
			}
		}
	}
}
