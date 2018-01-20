
#ifndef GAMEPLAY_GAMESTATE_MODELS_HPP
#define GAMEPLAY_GAMESTATE_MODELS_HPP

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"

#include <string>
#include <vector>

namespace gameplay
{
	namespace gamestate
	{
		struct Player
		{
			struct Skill
			{
				std::string name;
			};

			std::string name;

			std::vector<Skill> skills;
		};

		struct RecipeIngredient
		{
			using Ingredient = std::pair<RecipeIngredient *, unsigned>;

			const std::string name;
			const std::string desc;

			std::vector<Ingredient> ingredients;

			RecipeIngredient(std::string & name, std::string & desc)
				: name(name)
				, desc(desc)
			{}
		};

		using Dish = RecipeIngredient;
	}
}

#endif /* GAMEPLAY_GAMESTATE_MODELS_HPP */
