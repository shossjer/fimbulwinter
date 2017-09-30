
#include "catch.hpp"

#include "views_test.hpp"

#include <engine/gui/updater.hpp>

TEST_CASE("list updating", "[gui][list]")
{
	List view{
		engine::Entity{},
		engine::Asset{ "" },
		Gravity{},
		Margin{},
		Size{ { Size::TYPE::FIXED, 100.f },{ Size::TYPE::WRAP } },
		Layout{},
		ListData{
			"",
			Size{ { Size::TYPE::PARENT },{ Size::TYPE::WRAP } },
			Margin{},
			Gravity{},
			Layout{} } };

	REQUIRE(ViewTester::items(view) == 0);

	auto & change = ViewTester::change(view);
	change.clear();

	SECTION("size changed un-changed")
	{
		REQUIRE(!change.any());

		ViewUpdater::list(view, 0);
		REQUIRE(ViewTester::items(view) == 0);
		REQUIRE(change.affects_content());
		REQUIRE(!change.affects_size());
	}
	SECTION("size changed changed")
	{
		REQUIRE(!change.any());

		ViewUpdater::list(view, 2);
		REQUIRE(ViewTester::items(view) == 2);
		REQUIRE(change.affects_content());
		REQUIRE(change.affects_size());
	}
}

TEST_CASE("progress-bar updating", "[gui][progressBar]")
{
	TestView view{ Size{ { Size::TYPE::PERCENT } ,{ Size::TYPE::PERCENT } } };
	ProgressBar pb{ engine::Asset{""}, &view };

	auto & change = ViewTester::change(view);
	change.clear();

	SECTION("horizontal progress")
	{
		REQUIRE(!change.any());
		pb.direction = ProgressBar::Direction::HORIZONTAL;
		ViewUpdater::progress(pb, 50.f);

		REQUIRE(!change.affects_content());
		REQUIRE(change.affects_size());
		REQUIRE(view.get_size().width.meta == 50.f);
	}
	SECTION("vertical progress")
	{
		REQUIRE(!change.any());
		pb.direction = ProgressBar::Direction::VERTICAL;
		ViewUpdater::progress(pb, 50.f);

		REQUIRE(!change.affects_content());
		REQUIRE(change.affects_size());
		REQUIRE(view.get_size().height.meta == 50.f);
	}
}

TEST_CASE("view renderer-ring", "[gui][renderer]")
{
	TestView view{};

	auto & change = ViewTester::change(view);
	change.clear();

	REQUIRE(!ViewTester::should_render(view));
	REQUIRE(!ViewTester::is_rendered(view));
	REQUIRE(!ViewTester::is_invisible(view));

	SECTION("add to renderer")
	{
		REQUIRE(!change.any());
		ViewUpdater::renderer_add(view);

		REQUIRE(!ViewTester::is_invisible(view));
		REQUIRE(ViewTester::should_render(view));
		REQUIRE(change.affects_visibility());
	}
	SECTION("remove from renderer")
	{
		REQUIRE(!change.any());
		ViewTester::should_render(view) = true;

		ViewUpdater::renderer_remove(view);

		REQUIRE(!ViewTester::is_invisible(view));
		REQUIRE(!ViewTester::should_render(view));
		REQUIRE(change.affects_visibility());
	}
}

TEST_CASE("view in-visibility", "[gui][visibility]")
{
	TestView view{};

	auto & change = ViewTester::change(view);
	change.clear();

	SECTION("make invisible (not shown)")
	{
		REQUIRE(!change.any());
		REQUIRE(!ViewTester::should_render(view));
		REQUIRE(!ViewTester::is_rendered(view));
		REQUIRE(!ViewTester::is_invisible(view));

		ViewUpdater::visibility_hide(view);

		REQUIRE(ViewTester::is_invisible(view));
		REQUIRE(!ViewTester::should_render(view));
		REQUIRE(!change.any());
	}
	SECTION("make invisible (shown)")
	{
		REQUIRE(!change.any());
		ViewTester::should_render(view) = true;
		ViewTester::is_rendered(view) = true;
		REQUIRE(ViewTester::should_render(view));
		REQUIRE(ViewTester::is_rendered(view));
		REQUIRE(!ViewTester::is_invisible(view));

		ViewUpdater::visibility_hide(view);

		REQUIRE(ViewTester::is_invisible(view));
		REQUIRE(!ViewTester::should_render(view));
		REQUIRE(change.affects_size());
		REQUIRE(change.affects_visibility());
	}
	SECTION("make visible")
	{
		REQUIRE(!change.any());
		ViewTester::is_invisible(view) = true;

		REQUIRE(ViewTester::is_invisible(view));

		ViewUpdater::visibility_show(view);

		REQUIRE(!ViewTester::is_invisible(view));
		REQUIRE(change.affects_size());
		REQUIRE(change.affects_visibility());
	}
}