
#include "catch.hpp"

#include "gui_access.hpp"

#include <engine/gui/view_refresh.hpp>

// "refresh" hierarchy with "content updated" child (no group change)

// TODO:
// "refresh" hierarchy where group is also updated (nothing else affected)

// TODO:
// "refresh" hierarchy where group has been affected (by child) which affects another child.
//	Group (FIXED - Content)
//		Group (WRAP - Size)
//			Child (PARENT - Affected by Group size change)
//			Child (WRAP - Had the size update)

TEST_CASE("ViewRefresh - refresh content", "[gui][ViewRefresh]")
{
	View group1 = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } });
	View group2 = ViewAccess::create_group(
		Layout::HORIZONTAL,
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
		ViewRefresh::refresh(group1);

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
TEST_CASE("ViewRefresh - refresh parent offset", "[gui][ViewRefresh]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } });

	ViewAccess::change(view).clear();

	SECTION("Offset - changed")
	{
		ViewRefresh::refresh(view, Gravity{}, Offset{ height_t{ 100 }, width_t{ 200 } }, Size{});

		REQUIRE(ViewAccess::change(view).affects_moved());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_size());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
	SECTION("Offset - un-changed")
	{
		ViewAccess::offset(view) = Offset{ height_t{ 100 }, width_t{ 200 } };
		ViewRefresh::refresh(view, Gravity{}, Offset{ height_t{ 100 }, width_t{ 200 } }, Size{});

		REQUIRE(!ViewAccess::change(view).affects_moved());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_size());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
}
TEST_CASE("ViewRefresh - refresh parent re-sized", "[gui][ViewRefresh]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::PARENT },{ Size::PARENT } });

	ViewAccess::change(view).clear();

	SECTION("Size - changed")
	{
		ViewRefresh::refresh(
			view,
			Gravity{},
			Offset{},
			Size{ { Size::FIXED, height_t{ 50 } },{ Size::FIXED, width_t{ 100 } } });

		REQUIRE(ViewAccess::change(view).affects_size());
		REQUIRE(ViewAccess::size(view).height == height_t{ 50 });
		REQUIRE(ViewAccess::size(view).width == width_t{ 100 });

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_moved());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
	SECTION("Size - un-changed")
	{
		ViewAccess::size(view) =
			Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } };
		ViewRefresh::refresh(
			view,
			Gravity{},
			Offset{},
			Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } });

		REQUIRE(!ViewAccess::change(view).affects_size());

		// unaffected
		REQUIRE(!ViewAccess::change(view).affects_moved());
		REQUIRE(!ViewAccess::change(view).affects_visibility());
	}
}

TEST_CASE("ViewRefresh - offset {parent}", "[gui][ViewRefresh][Offset]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::PARENT },{ Size::PARENT } },
		nullptr,
		Margin{ width_t{10}, width_t{5}, height_t{20}, height_t{25} });

	ViewRefresh::refresh(
		view,
		Gravity{ Gravity::HORIZONTAL_LEFT | Gravity::VERTICAL_CENTRE },	// should not affect test
		Offset{ height_t{ 10 }, width_t{ 40 } },
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 200 } } });

	SECTION("Offset - Vertical")
	{
		REQUIRE(view.offset.height == height_t{ 10 + 20 });
		REQUIRE(view.offset.width == width_t{ 40 + 10 });
		REQUIRE(ViewAccess::size(view).height == height_t{ 100 - 20 - 25 });
		REQUIRE(ViewAccess::size(view).width == width_t{ 200 - 10 - 5 });
	}
}
TEST_CASE("ViewRefresh - offset {fixed}", "[gui][ViewRefresh][Offset]")
{
	const uint32_t HEIGHT = 20;
	const uint32_t WIDTH = 40;

	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ HEIGHT } },{ Size::FIXED, width_t{ WIDTH }} },
		nullptr,
		Margin{ width_t{ 1 }, width_t{ 2 }, height_t{ 3 }, height_t{ 4 } });

	const uint32_t PARENT_HEIGHT = 100;
	const uint32_t PARENT_WIDTH = 200;
	const auto parent_size = Size{ { Size::FIXED, height_t{ PARENT_HEIGHT } },{ Size::FIXED, width_t{ PARENT_WIDTH } } };
	const uint32_t OFFSET_TOP = 10;
	const uint32_t OFFSET_LEFT = 40;
	const auto parent_offset = Offset{ height_t{ OFFSET_TOP }, width_t{ OFFSET_LEFT } };

	SECTION("Top / Left")
	{
		ViewRefresh::refresh(
			view,
			Gravity{ Gravity::HORIZONTAL_LEFT | Gravity::VERTICAL_TOP },
			parent_offset, parent_size);

		REQUIRE(view.offset.height == height_t{ OFFSET_TOP + 3 });
		REQUIRE(view.offset.width == width_t{ OFFSET_LEFT + 1 });
	}
	SECTION("Centre / Centre")
	{
		view.gravity = Gravity{ Gravity::HORIZONTAL_CENTRE | Gravity::VERTICAL_CENTRE };

		ViewRefresh::refresh(
			view,
			Gravity::unmasked(),
			parent_offset, parent_size);

		REQUIRE(view.offset.height == height_t{ (OFFSET_TOP + (PARENT_HEIGHT - HEIGHT)/2 + (3 - 4)) });
		REQUIRE(view.offset.width == width_t{ (OFFSET_LEFT + (PARENT_WIDTH - WIDTH)/2 + (1 - 2)) });
	}
	SECTION("Bottom / Right")
	{
		view.gravity = Gravity{ Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_BOTTOM };

		ViewRefresh::refresh(
			view,
			Gravity{ Gravity::HORIZONTAL_RIGHT | Gravity::VERTICAL_BOTTOM }, // only allow bottom / right
			parent_offset, parent_size);

		REQUIRE(view.offset.height == height_t{ OFFSET_TOP + (PARENT_HEIGHT - HEIGHT) - 4 });
		REQUIRE(view.offset.width == width_t{ OFFSET_LEFT + (PARENT_WIDTH - WIDTH) - 2 });
	}
}
