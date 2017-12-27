
#include "catch.hpp"

#include "gui_access.hpp"

#include <engine/gui/noodle.hpp>

using namespace engine::gui::data;

namespace
{
	void populate(Nodes & nodes)
	{
		Value main = utility::in_place_type<KeyValues>;
		KeyValues & player = utility::get<KeyValues>(main);


		player.data.emplace_back(
			KeyValue{ Asset{ "name" }, utility::in_place_type<std::string> });

		player.data.emplace_back(
			KeyValue{ Asset{ "skills" }, utility::in_place_type<Values> });
		Values & skills = utility::get<Values>(player.data.back().second);
		skills.data.emplace_back(utility::in_place_type<KeyValues>);
		KeyValues & skill0 = utility::get<KeyValues>(skills.data.back());

		skill0.data.emplace_back(
			KeyValue{ Asset{ "name" }, utility::in_place_type<std::string> });

		visit(ParseSetup{ Asset{ "player" }, nodes }, main);
	}
}

TEST_CASE("Data-node setup", "[gui][node]")
{
	Nodes nodes{};

	SECTION("Node setup")
	{
		populate(nodes);

		REQUIRE(nodes.size() == 1);
		REQUIRE(nodes.back().first == Asset{"player"});
		auto & player = nodes.back().second;
		REQUIRE(player.nodes.size() == 2);
		REQUIRE(player.nodes[0].first == Asset{ "name" });
		REQUIRE(player.nodes[1].first == Asset{ "skills" });
		auto & skills = player.nodes[1].second;
		REQUIRE(skills.nodes.size() == 1);
		REQUIRE(skills.nodes[0].first == Asset{ "0" });
		auto & skill0 = skills.nodes[0].second;
		REQUIRE(skill0.nodes.size() == 1);
		REQUIRE(skill0.nodes[0].first == Asset{ "name" });
	}
}
