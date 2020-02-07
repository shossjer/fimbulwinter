#include "gui_access.hpp"

#include "engine/gui/update.hpp"

#include <catch/catch.hpp>

namespace engine
{
	namespace gui
	{
		struct ViewTester
		{
			template<typename D, typename T>
			static D wrap_content(const T & content)
			{
				return ViewUpdater::wrap_content(utility::in_place_type<D>, content);
			}
		};
	}
}

TEST_CASE("ViewUpdater - Wrap Group", "[gui][ViewUpdater][wrap][group]")
{
	View group = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } });
	View child1 = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::FIXED, width_t{ 50 } } },
		&group);
	View child2 = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::FIXED, height_t{ 400 } },{ Size::FIXED, width_t{ 200 } } },
		&group);

	View::Group & group_content = ViewUpdater::content<View::Group>(group);
	group_content.children.push_back(&child1);
	group_content.children.push_back(&child2);

	SECTION("Group wrap size - Horizontal (Layout::Horizontal)")
	{
		group_content.layout = Layout::HORIZONTAL;

		auto height = ViewTester::wrap_content<height_t>(group_content);
		auto width = ViewTester::wrap_content<width_t>(group_content);

		REQUIRE(height == height_t{ 400 });
		REQUIRE(width == width_t{ 50 + 200 });
	}
	SECTION("Group wrap size - Vertical")
	{
		group_content.layout = Layout::VERTICAL;

		auto height = ViewTester::wrap_content<height_t>(group_content);
		auto width = ViewTester::wrap_content<width_t>(group_content);

		REQUIRE(height == height_t{ 100 + 400 });
		REQUIRE(width == width_t{ 200 });
	}
	SECTION("Group wrap size - Relative")
	{
		group_content.layout = Layout::RELATIVE;

		auto height = ViewTester::wrap_content<height_t>(group_content);
		auto width = ViewTester::wrap_content<width_t>(group_content);

		REQUIRE(height == height_t{ 400 });
		REQUIRE(width == width_t{ 200 });
	}
}
TEST_CASE("ViewUpdater - Wrap Text", "[gui][ViewUpdater][wrap][text]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text> },
		Size{ { Size::WRAP },{ Size::WRAP } },
		nullptr);

	View::Text & content = ViewUpdater::content<View::Text>(view);
	content.display = "1234 6789";

	auto height = ViewTester::wrap_content<height_t>(content);
	auto width = ViewTester::wrap_content<width_t>(content);

	SECTION("Text wrap size")
	{
		REQUIRE(height == height_t{ 6 });
		REQUIRE(width == width_t{ 9 * 6 });
	}
}

SCENARIO("ViewUpdater - Parent update", "[gui][ViewUpdater][parent]")
{
	View group = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } });
	View child = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text>, "1234 6789" },
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group);

	View::Text & child_content = ViewUpdater::content<View::Text>(child);
	View::Group & group_content = ViewUpdater::content<View::Group>(group);
	group_content.children.push_back(&child);

	ViewAccess::change(group).clear();

	auto change = ViewUpdater::update(child, child_content);

	GIVEN("a WRAP size 'group' with a 'size updated' child.")
	{
		ViewAccess::size(group) = Size{ { Size::WRAP },{ Size::WRAP } };

		WHEN("the 'child' view informs the parent of its update.")
		{
			ViewUpdater::parent(child, change);

			THEN("the parent 'group' should also be updated.")
			{
				auto group_change = ViewAccess::change(group);
				REQUIRE(group_change.affects_size_h());
				REQUIRE(group_change.affects_size_w());
				REQUIRE(group_change.affects_content());
			}
		}
	}
	GIVEN("a FIXED size 'group' with a 'size updated' child.")
	{
		ViewAccess::size(group) = Size{ { Size::FIXED, height_t{ 200 } },{ Size::FIXED, width_t{ 200 } } };

		WHEN("the 'child' view informs the parent of its update.")
		{
			ViewUpdater::parent(child, change);

			THEN("the parent 'group' should be marked for change but not its size.")
			{
				auto group_change = ViewAccess::change(group);
				REQUIRE(!group_change.affects_size_h());
				REQUIRE(!group_change.affects_size_w());
				REQUIRE(group_change.affects_content());
			}
		}
	}
}

SCENARIO("ViewUpdater - Parent(s) update", "[gui][ViewUpdater][parent]")
{
	View group1 = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } });
	View group2 = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group1);
	View group3 = ViewAccess::create_group(
		Layout::HORIZONTAL,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group2);
	View child = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text>, "1234 6789" },
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group3);

	auto & child_content = ViewUpdater::content<View::Text>(child);
	ViewUpdater::content<View::Group>(group1).children.push_back(&group2);
	ViewUpdater::content<View::Group>(group2).children.push_back(&group3);
	ViewUpdater::content<View::Group>(group3).children.push_back(&child);

	ViewAccess::change(group1).clear();
	ViewAccess::change(group2).clear();
	ViewAccess::change(group3).clear();

	auto child_change = ViewUpdater::update(child, child_content);

	GIVEN("a hierarchy of 'group's which are all 'WRAP'.")
	{
		WHEN("a child is updated")
		{
			ViewUpdater::parent(child, child_change);

			THEN("all the groups should have size updated.")
			{
				{
					auto group_change = ViewAccess::change(group1);
					REQUIRE(group_change.affects_size_h());
					REQUIRE(group_change.affects_size_w());
					REQUIRE(group_change.affects_content());
				}
				{
					auto group_change = ViewAccess::change(group2);
					REQUIRE(group_change.affects_size_h());
					REQUIRE(group_change.affects_size_w());
					REQUIRE(group_change.affects_content());
				}
				{
					auto group_change = ViewAccess::change(group3);
					REQUIRE(group_change.affects_size_h());
					REQUIRE(group_change.affects_size_w());
					REQUIRE(group_change.affects_content());
				}
			}
		}
	}
	GIVEN("a hierarchy of 'group's, some 'wrap'.")
	{
		ViewAccess::size(group2) = Size{ { Size::FIXED, height_t{ 200 } },{ Size::FIXED, width_t{ 200 } } };

		WHEN("a child is updated")
		{
			ViewUpdater::parent(child, child_change);

			THEN("group 'wrapping' the child should be updated.")
			{
				auto group_change = ViewAccess::change(group3);
				REQUIRE(group_change.affects_size_h());
				REQUIRE(group_change.affects_size_w());
				REQUIRE(group_change.affects_content());
			}
			THEN("fixed 'group' should be marked for change but not its size.")
			{
				auto group_change = ViewAccess::change(group2);
				REQUIRE(!group_change.affects_size_h());
				REQUIRE(!group_change.affects_size_w());
				REQUIRE(group_change.affects_content());
			}
			THEN("base wrap 'group' should be marked for change but not its size.")
			{
				auto group_change = ViewAccess::change(group1);
				REQUIRE(!group_change.affects_size_h());
				REQUIRE(!group_change.affects_size_w());
				REQUIRE(group_change.affects_content());
			}
		}
	}
}

TEST_CASE("ViewUpdater::update<View::Text>", "[gui][ViewUpdater][child]")
{
	View view = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text>, "1234 6789" },
		Size{ { Size::WRAP },{ Size::WRAP } });

	ViewAccess::change(view).clear();
	View::Text & content = ViewUpdater::content<View::Text>(view);

	// A View (TEXT) will be marked as 'HORIZONTAL' size changed if
	// updated with different length test. If it wraps content.
	SECTION("View (WRAP) changed size")
	{
		auto update_change = ViewUpdater::update(view, content);
		auto child_change = ViewAccess::change(view);

		REQUIRE(child_change.affects_size_h());
		REQUIRE(child_change.affects_size_w());
		REQUIRE(child_change.affects_content());

		REQUIRE(update_change.affects_size_h());
		REQUIRE(update_change.affects_size_w());
		REQUIRE(update_change.affects_content());
	}

	// A View will only have size change if new content makes it so.
	SECTION("View (WRAP) Not changed size")
	{
		ViewAccess::size(view) = Size{ { Size::WRAP, height_t{ 6 } },{ Size::WRAP, width_t{ 6 * 9 } } };
		auto update_change = ViewUpdater::update(view, content);
		auto child_change = ViewAccess::change(view);

		REQUIRE(!child_change.affects_size_h());
		REQUIRE(!child_change.affects_size_w());
		REQUIRE(child_change.affects_content());

		REQUIRE(!update_change.affects_size_h());
		REQUIRE(!update_change.affects_size_w());
		REQUIRE(update_change.affects_content());
	}
	// A View with 'size' PARENT should not have its size affected
	// by content change.
	SECTION("View (PARENT) Not changed size")
	{
		ViewAccess::size(view) = Size{ { Size::FIXED, height_t{ 200 } },{ Size::FIXED, width_t{ 200 } } };
		auto update_change = ViewUpdater::update(view, content);
		auto child_change = ViewAccess::change(view);

		REQUIRE(!child_change.affects_size_h());
		REQUIRE(!child_change.affects_size_w());
		REQUIRE(child_change.affects_content());

		REQUIRE(!update_change.affects_size_h());
		REQUIRE(!update_change.affects_size_w());
		REQUIRE(update_change.affects_content());
	}
}
