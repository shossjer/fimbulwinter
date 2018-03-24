
#ifndef GAMEPLAY_RECIPES_HPP
#define GAMEPLAY_RECIPES_HPP

#include "core/serialize.hpp"

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
		template <typename S>
		friend void serialize_class(S & s, Ingredient & x)
		{
			using core::serialize;

			serialize(s, "name", x.name);
			serialize(s, "quantity", x.quantity);
		}
	};

	class Recipe
	{
	public:
		std::string name;
		int durability;
		utility::optional<int> time;
		utility::optional<std::vector<Ingredient>> ingredients;

	public:
		template <typename S>
		friend void serialize_class(S & s, Recipe & x)
		{
			using core::serialize;

			serialize(s, "name", x.name);
			serialize(s, "durability", x.durability);
			serialize(s, "time", x.time);
			serialize(s, "ingredients", x.ingredients);
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
		template <typename S>
		friend void serialize_class(S & s, Recipes & x)
		{
			using core::serialize;

			serialize(s, "recipes", x.recipes);
		}
	};
}

#endif /* GAMEPLAY_RECIPES_HPP */
