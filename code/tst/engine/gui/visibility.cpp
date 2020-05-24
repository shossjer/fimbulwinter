#include "gui_access.hpp"

#include "engine/gui/view_refresh.hpp"
#include "engine/gui/update.hpp"

#include <catch2/catch.hpp>

SCENARIO("Adding view (show) to already updated group.", "[gui][Visibility]")
{
	// this test verified a bug when working with tabs.
	View group1 = ViewAccess::create_group(
		Layout::RELATIVE,
		Size{ { Size::FIXED, height_t{ 200 } },{ Size::FIXED, width_t{ 400 } } });
	View group2 = ViewAccess::create_group(
		Layout::RELATIVE,
		Size{ { Size::PARENT },{ Size::PARENT } },
		&group1);

	ViewUpdater::update(group1, ViewUpdater::content<View::Group>(group1));
	ViewUpdater::update(group2, ViewUpdater::content<View::Group>(group2));
	ViewUpdater::content<View::Group>(group1).adopt(&group2);
	ViewRefresh::refresh(group1);

	REQUIRE(!group1.change.any());
	REQUIRE(!group2.change.any());

	GIVEN("a PARENT size 'child', which is added to the prev. calculated parent.")
	{
		View child = ViewAccess::create_child(
			View::Content{ utility::in_place_type<View::Color> },
			Size{ { Size::PARENT },{ Size::PARENT } },
			&group2);

		// child is "added" to the views.
		ViewUpdater::show(child);

		REQUIRE(group1.change.any());
		REQUIRE(group2.change.any());
		REQUIRE(child.size.height == height_t{ 0 });
		REQUIRE(child.size.width == width_t{ 0 });

		WHEN("the views are refreshed.")
		{
			ViewRefresh::refresh(group1);

			THEN("the child's size should be updated")
			{
				REQUIRE(child.size.height == height_t{ 200 });
				REQUIRE(child.size.width == width_t{ 400 });
			}
		}
	}
}
