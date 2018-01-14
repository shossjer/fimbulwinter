
#ifndef GAMEPLAY_GAMESTATE_DATA_HPP
#define GAMEPLAY_GAMESTATE_DATA_HPP

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/gui/gui.hpp"

#include <vector>

namespace gameplay
{
	namespace gamestate
	{
		struct PlayerData
		{
			using Value = engine::gui::data::Value;
			using Values = engine::gui::data::Values;
			using KeyValue = engine::gui::data::KeyValue;
			using KeyValues = engine::gui::data::KeyValues;

			std::string name;

			struct Skill
			{
				std::string name;
			};

			std::vector<Skill> skills;

			KeyValue message() const
			{
				KeyValue main{ engine::Asset{ "player" }, utility::in_place_type<KeyValues> };
				KeyValues & player = utility::get<KeyValues>(main.second);

				player.data.push_back(
					KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, this->name } });

				player.data.push_back(
					KeyValue{ engine::Asset{ "skills" }, utility::in_place_type<Values> });
				Values & skills = utility::get<Values>(player.data.back().second);

				skills.data.reserve(this->skills.size());

				for (auto & skill : this->skills)
				{
					skills.data.emplace_back(utility::in_place_type<KeyValues>);

					auto & skill_map = utility::get<KeyValues>(skills.data.back());

					skill_map.data.push_back(
						KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, skill.name } });
				}

				return main;
			}
		};

		struct RecipeData
		{
			using Value = engine::gui::data::Value;
			using Values = engine::gui::data::Values;
			using KeyValue = engine::gui::data::KeyValue;
			using KeyValues = engine::gui::data::KeyValues;

			struct Recipe
			{
				std::string name;
			};

			std::vector<Recipe> recipes;

			KeyValue message() const
			{
				KeyValue main{ engine::Asset{ "game" }, utility::in_place_type<KeyValues> };
				KeyValues & game = utility::get<KeyValues>(main.second);

				game.data.push_back(
					KeyValue{ engine::Asset{ "recipes" }, utility::in_place_type<Values> });
				Values & recipes = utility::get<Values>(game.data.back().second);

				recipes.data.reserve(this->recipes.size());

				for (auto & recipe : this->recipes)
				{
					recipes.data.emplace_back(utility::in_place_type<KeyValues>);

					auto & recipe_map = utility::get<KeyValues>(recipes.data.back());

					recipe_map.data.push_back(
						KeyValue{ engine::Asset{ "name" },{ utility::in_place_type<std::string>, recipe.name } });
				}

				return main;
			}
		};
	}
}

#endif /* GAMEPLAY_GAMESTATE_DATA_HPP */
