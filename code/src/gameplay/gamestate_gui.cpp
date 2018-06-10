
#include "gamestate_gui.hpp"

#include <engine/gui/gui.hpp>

namespace gameplay
{
	namespace gamestate
	{
		using Value = engine::gui::data::Value;
		using Values = engine::gui::data::Values;
		using KeyValue = engine::gui::data::KeyValue;
		using KeyValues = engine::gui::data::KeyValues;

		KeyValue encode_gui(const Player & data)
		{
			KeyValue message{ engine::Asset{ "player" }, utility::in_place_type<KeyValues> };
			KeyValues & player = utility::get<KeyValues>(message.second);

			player.data.push_back(
				KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, data.name } });

			player.data.push_back(
				KeyValue{ engine::Asset{ "skills" }, utility::in_place_type<Values> });
			Values & skills = utility::get<Values>(player.data.back().second);

			skills.data.reserve(data.skills.size());

			for (auto & skill : data.skills)
			{
				skills.data.emplace_back(utility::in_place_type<KeyValues>);

				auto & skill_map = utility::get<KeyValues>(skills.data.back());

				skill_map.data.push_back(
					KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, skill.name } });
			}

			return message;
		}

		KeyValue encode_gui(const std::vector<const Dish *> & dishes)
		{
			// temp
			debug_assert(dishes.size() >= 4);

			KeyValue message{ engine::Asset{ "hell" }, utility::in_place_type<KeyValues> };
			KeyValues & hell = utility::get<KeyValues>(message.second);
			hell.data.push_back(KeyValue{ engine::Asset{ "dishes" }, utility::in_place_type<Values> });
			Values & dishes_data = utility::get<Values>(hell.data.back().second);

			dishes_data.data.reserve(dishes.size());

			for (auto & dish : dishes)
			{
				dishes_data.data.emplace_back(utility::in_place_type<KeyValues>);
				auto & dish_data = utility::get<KeyValues>(dishes_data.data.back());

				dish_data.data.push_back(
					KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, dish->name } });

				// List raw materials and quantiy

				// Sorted list of cooking steps

			}

			return message;
		}
	}
}
