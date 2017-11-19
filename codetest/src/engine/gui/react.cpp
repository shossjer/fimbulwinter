
#include "catch.hpp"

#include "gui_access.hpp"

#include <engine/gui/measure.hpp>
#include <engine/gui/react.hpp>
#include <engine/gui/update.hpp>

namespace
{
	bool resource_added = false;
}

SCENARIO("Adding items to list - Size", "[gui][Visibility]")
{
	if (!resource_added)
	{
		resource_added = true;
		resource::put(engine::Asset::null(), 0);
	}
	auto group = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP },{ Size::WRAP } });

	auto function = ViewAccess::create_function(
		Function::Content
		{
			utility::in_place_type<Function::List>,
			DataVariant
			{
				utility::in_place_type<TextData>,
				"Name",
				Size{},
				Margin{ width_t{}, width_t{}, height_t{5}, height_t{5}},
				Gravity{},
				engine::Asset::null(),
				"Display"
			},
		},
		group);

	GIVEN("an view with list function")
	{
		ViewMeasure::refresh(group);

		REQUIRE(group.size.height == height_t{ 0 });
		REQUIRE(group.size.width == width_t{ 0 });

		WHEN("the list is updated with items.")
		{
			std::vector<data::Value> items;
			items.emplace_back(utility::in_place_type<std::string>, "str1");
			items.emplace_back(utility::in_place_type<std::string>, "str2");
			items.emplace_back(utility::in_place_type<std::string>, "str3");
			ListReaction{ data::Values{ items } }(function);

			ViewMeasure::refresh(group);

			THEN("the view should have its size updated.")
			{
				auto & group_content = utility::get<View::Group>(group.content);

				REQUIRE(group_content.children.size() == 3);

			//	list items are no longer updated by "List Reaction"
			//	REQUIRE(group.size.height == height_t{ (2*5 + 6)*3 });
			//	REQUIRE(group.size.width == width_t{ 4*6 });
			}
		}
	}
}
SCENARIO("Adding items to list - Offset", "[gui][Visibility]")
{
	if (!resource_added)
	{
		resource_added = true;
		resource::put(engine::Asset::null(), 0);
	}
	auto base = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::FIXED, height_t{500} },{ Size::FIXED, width_t{500} } });
	// makes some distance
	auto spacing = ViewAccess::create_group(
		View::Group::Layout::RELATIVE,
		Size{ { Size::FIXED, height_t{ 100 } },{ Size::PARENT } },
		&base);

	auto group = ViewAccess::create_group(
		View::Group::Layout::VERTICAL,
		Size{ { Size::WRAP },{ Size::WRAP } },
		&base,
		Margin{ width_t{ 100 }, width_t{}, height_t{ 200 }, height_t{} });

	ViewUpdater::content<View::Group>(base).children.push_back(&spacing);
	ViewUpdater::content<View::Group>(base).children.push_back(&group);

	auto function = ViewAccess::create_function(
		Function::Content
		{
			utility::in_place_type<Function::List>,
			DataVariant
			{
				utility::in_place_type<TextData>,
				"Name",
				Size{},
				Margin{ width_t{}, width_t{}, height_t{5}, height_t{5}},
				Gravity{},
				engine::Asset::null(),
				"Display"
			},
		},
		group);

	auto & list = utility::get<Function::List>(function.content);

	GIVEN("an view with list function")
	{
		ViewMeasure::refresh(base);

		WHEN("the list is updated with items.")
		{
			std::vector<data::Value> items;
			items.emplace_back(utility::in_place_type<std::string>, "str1");
			items.emplace_back(utility::in_place_type<std::string>, "str2");
			items.emplace_back(utility::in_place_type<std::string>, "str3");
			ListReaction{ data::Values{ items } }(function);

			ViewMeasure::refresh(base);

			THEN("the view should have its size updated.")
			{
				auto & gc = utility::get<View::Group>(group.content);

				REQUIRE(gc.children[0]->offset.height == height_t{ 100 + 200 + 5 });
				REQUIRE(gc.children[0]->offset.width == width_t{ 100 });
			}
		}
	}
}
