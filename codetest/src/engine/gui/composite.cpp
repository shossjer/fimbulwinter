
#include "catch.hpp"

#include "gui_access.hpp"

#include <engine/gui/creation.hpp>
#include <engine/gui/measure.hpp>
#include <engine/gui/update.hpp>

// Test to verify a PARENT sized Group with VERTICAL (or HORIZONTAL) layout;
// will have its childrens offset updated if one of the children changes its size.
// This updated child needs to be !last in the layout.
// Note: Depth is currenly not checked since it is managed through Creator.
TEST_CASE("Change child size in PARENT Group, should affect other childen.", "[gui][ViewUpdater][parent]")
{
	View group0 = ViewAccess::create_group(
		View::Group::Layout::HORIZONTAL,
		Size{ { Size::FIXED, height_t{ 400 } },{ Size::FIXED, width_t{ 600 } } });
	View group1 = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::PARENT },{ Size::PARENT } },	// PARENT infortant for the test
		&group0);
	View group2 = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group1);
	View group2a = ViewAccess::create_group(	// child view will be added here
		View::Group::Layout::RELATIVE,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&group2);
	View group2b = ViewAccess::create_group(	// this view should be offsetted after Group2 changes size
		View::Group::Layout::HORIZONTAL,
		Size{ { Size::FIXED, height_t{100} }, { Size::FIXED, width_t{200} } },
		&group2);

	utility::get<View::Group>(group0.content).adopt(&group1);
	utility::get<View::Group>(group1.content).adopt(&group2);
	utility::get<View::Group>(group2.content).adopt(&group2a);
	utility::get<View::Group>(group2.content).adopt(&group2b);

	ViewUpdater::update(group2b, utility::get<View::Group>(group2b.content));
	ViewUpdater::update(group2a, utility::get<View::Group>(group2a.content));
	ViewUpdater::update(group2, utility::get<View::Group>(group2.content));
	ViewUpdater::update(group1, utility::get<View::Group>(group1.content));
	ViewUpdater::update(group0, utility::get<View::Group>(group0.content));

	ViewMeasure::refresh(group0);

	REQUIRE(!group0.change.any());
	REQUIRE(!group1.change.any());
	REQUIRE(!group2.change.any());
	REQUIRE(!group2a.change.any());
	REQUIRE(!group2b.change.any());

	REQUIRE(group2.size.height == height_t{ 100 });
	REQUIRE(group2.size.width == width_t{ 200 });

	SECTION("Verify offset of existing child has been updated.")
	{
		View child = ViewAccess::create_child(
			View::Content{ utility::in_place_type<View::Text>, "" },
			Size{ { Size::FIXED, height_t{20} },{ Size::FIXED, width_t{100}} },
			&group2a);

		auto & child_content = ViewUpdater::content<View::Text>(child);

		// Add a child to the group to make it change its size.
		utility::get<View::Group>(group2a.content).adopt(&child);

		auto child_change = ViewUpdater::update(child, child_content);
		ViewUpdater::creation(group2a);

		REQUIRE(group2a.size.height == height_t{20});
		REQUIRE(group2a.size.width == width_t{100});

		ViewMeasure::refresh(group1);

		REQUIRE(group2.size.height == height_t{ 100 + 20 });
		REQUIRE(group2.size.width == width_t{ 200 });

		// the bottom most view should be pushed down by the new view.
		REQUIRE(group2b.offset.height == height_t{ 20 });
	}
}

TEST_CASE("Update parent size after adding child", "[gui][ViewUpdater][parent]")
{
	View group = ViewAccess::create_group(	// child view will be added here
		View::Group::Layout::RELATIVE,
		Size{ { Size::WRAP },{ Size::WRAP } });

	ViewUpdater::update(group, utility::get<View::Group>(group.content));

	ViewMeasure::refresh(group);

	REQUIRE(!group.change.any());

	REQUIRE(group.size.height == height_t{ 0 });
	REQUIRE(group.size.width == width_t{ 0 });

	SECTION("")
	{
		View child = ViewAccess::create_child(
			View::Content{ utility::in_place_type<View::Text>, "" },
			Size{ { Size::FIXED, height_t{ 20 } },{ Size::FIXED, width_t{ 100 } } },
			&group);

		auto & child_content = ViewUpdater::content<View::Text>(child);

		// Add a child to the group to make it change its size.
		utility::get<View::Group>(group.content).adopt(&child);

		ViewUpdater::update(child, child_content);
		ViewUpdater::creation(group);

		REQUIRE(group.size.height == height_t{ 20 });
		REQUIRE(group.size.width == width_t{ 100 });

		ViewMeasure::refresh(group);
	}
}

// This test verified a problem with an item under a list was not offsetted
// after the list was updated
TEST_CASE("View{Group} with Function{List} should WRAP dynamic content", "[list][wrap]")
{
	resource::put("make_this_better", 0);

	auto base = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP },{ Size::FIXED, width_t{ 100 } }, });

	auto group = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP }, { Size::FIXED, width_t{ 100 } }, },
		&base);

	auto child_group = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP },{ Size::WRAP }, },
		&base);
	auto child_text = ViewAccess::create_child(
		View::Content{ utility::in_place_type<View::Text>, "make_this_better" },
		Size{ { Size::FIXED, height_t{ 20 } },{ Size::FIXED, width_t{ 100 } } },
		&child_group);

	utility::get<View::Group>(base.content).adopt(&group);
	utility::get<View::Group>(base.content).adopt(&child_group);
	utility::get<View::Group>(child_group.content).adopt(&child_text);

	auto function = ViewAccess::create_function(
		Function::Content
		{
			utility::in_place_type<Function::List>,
			DataVariant
			{
				utility::in_place_type<TextData>,
				"Name",
				Size{},
				Margin{},
				Gravity{},
				"make_this_better",
				"Display"
			},
		},
		group);

	auto & group_content = utility::get<View::Group>(group.content);
	auto & list = utility::get<Function::List>(function.content);

	GIVEN("an view with list function")
	{
		ViewMeasure::refresh(base);

		REQUIRE(group.size.height == height_t{ 0 });
		REQUIRE(group.size.width == width_t{100});

		REQUIRE(child_text.offset.height == height_t{ 0 });
		REQUIRE(child_text.offset.width == width_t{ 0 });

		WHEN("the list is updated with items.")
		{
			std::vector<data::Value> items;
			items.emplace_back(utility::in_place_type<std::string>, "str1");
			items.emplace_back(utility::in_place_type<std::string>, "str2");
			items.emplace_back(utility::in_place_type<std::string>, "str3");
			ListReaction{ data::Values{ items } }(function);

			REQUIRE(base.change.affects_size());

			ViewMeasure::refresh(base);

			auto child_height = group_content.children[0]->size.height;
			REQUIRE(child_height.value > 0);

			THEN("the list view should have its size updated.")
			{
				REQUIRE(group.size.height == height_t{ child_height.value * 3 });
				REQUIRE(group.size.width == width_t{ 100 });

				REQUIRE(child_text.offset.height == height_t{ child_height.value * 3 });
				REQUIRE(child_text.offset.width == width_t{ 0 });
			}
		}
	}
}