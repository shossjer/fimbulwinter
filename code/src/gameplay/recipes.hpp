
#ifndef GAMEPLAY_RECIPES_HPP
#define GAMEPLAY_RECIPES_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

#include "utility/optional.hpp"

#include <string>
#include <vector>

namespace gameplay
{
	class Ingredient
	{
	public:
		std::string name;
		int quantity;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &Ingredient::name),
				std::make_pair(utility::string_view("quantity"), &Ingredient::quantity)
				);
		}
	};

	class Recipe
	{
	public:
		std::string name;
		int durability;
		utility::optional<int> time;
		std::vector<Ingredient> ingredients;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &Recipe::name),
				std::make_pair(utility::string_view("durability"), &Recipe::durability),
				std::make_pair(utility::string_view("time"), &Recipe::time),
				std::make_pair(utility::string_view("ingredients"), &Recipe::ingredients)
				);
		}
	};

	class Recipes
	{
	private:
		std::vector<Recipe> recipes;

	public:
		Recipes() = default;
		Recipes(const Recipes &) = delete;
		Recipes & operator = (const Recipes &) = delete;

	public:
		int find(const std::string & name) const
		{
			for (int i = 0; i < recipes.size(); i++)
			{
				if (recipes[i].name == name)
					return i;
			}
			return -1;
		}
		const Recipe & get(int i) const { return recipes[i]; }
		int index(const Recipe & recipe) const { return static_cast<int>(&recipe - recipes.data()); }
		int size() const { return recipes.size(); }

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("recipes"), &Recipes::recipes)
				);
		}
	};
}

#endif /* GAMEPLAY_RECIPES_HPP */
