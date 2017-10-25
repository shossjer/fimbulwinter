
#include "catch.hpp"

#include "gui_access.hpp"

#include <engine/gui2/measure.hpp>

// "refresh" hierarchy with "content updated" child (no group change)

// TODO:
// "refresh" hierarchy where group is also updated (nothing else affected)

// TODO:
// "refresh" hierarchy where group has been affected (by child) which affects another child.
//	Group (FIXED - Content)
//		Group (WRAP - Size)
//			Child (PARENT - Affected by Group size change)
//			Child (WRAP - Had the size update)

TEST_CASE("ViewMeasure - refresh content", "[gui2][ViewMeasure]")
{
	View group1 = ViewAccess::create_group(
		View::Group::Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } });
	View group2 = ViewAccess::create_group(
		View::Group::Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group1);
	View child = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } },
		&group2);

	ViewAccess::change(group1).clear();
	ViewAccess::change(group1).set_content();
	ViewAccess::change(group2).clear();
	ViewAccess::change(group2).set_content();
	ViewAccess::change(child).clear();
	ViewAccess::change(child).set_content();

	ViewAccess::content<View::Group>(group1).adopt(&group2);
	ViewAccess::content<View::Group>(group2).adopt(&child);

	SECTION("After update; changes should be cleared for all.")
	{
		ViewMeasure::refresh(group1);

		REQUIRE(!ViewAccess::change(group1).any());
		REQUIRE(!ViewAccess::change(group2).any());
		REQUIRE(!ViewAccess::change(child).any());
	}
}

/**
	Verify - refresh for changed parent
		Offset
			Changed
			Unaffected
		PARENT (H / V)
			Same change - not set
			Parent size updated - change set
		FIXED/WRAP
			Unaffected
 */
TEST_CASE("ViewMeasure - refresh parent offset", "[gui2][ViewMeasure]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } });

	ViewAccess::change(view).clear();

	SECTION("Offset - changed")
	{
		ViewMeasure::refresh(view, Offset{ height_t{ 100 }, width_t{ 200 } }, Size{});

		REQUIRE(ViewAccess::change(view).affects_offset());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_size());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
	SECTION("Offset - un-changed")
	{
		ViewAccess::offset(view) = Offset{ height_t{ 100 }, width_t{ 200 } };
		ViewMeasure::refresh(view, Offset{ height_t{ 100 }, width_t{ 200 } }, Size{});

		REQUIRE(!ViewAccess::change(view).affects_offset());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_size());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
}
TEST_CASE("ViewMeasure - refresh parent re-sized", "[gui2][ViewMeasure]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::PARENT },{ Size::PARENT } });

	ViewAccess::change(view).clear();

	SECTION("Size - changed")
	{
		ViewMeasure::refresh(
			view,
			Offset{},
			Size{ { Size::FIXED, height_t{ 50 } },{ Size::FIXED, width_t{ 100 } } });

		REQUIRE(ViewAccess::change(view).affects_size());
		REQUIRE(ViewAccess::size(view).height == height_t{ 50 });
		REQUIRE(ViewAccess::size(view).width == width_t{ 100 });

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_offset());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
	SECTION("Size - un-changed")
	{
		ViewAccess::size(view) =
			Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } };
		ViewMeasure::refresh(
			view,
			Offset{},
			Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } });

		REQUIRE(!ViewAccess::change(view).affects_size());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_offset());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
}
