
#ifndef GAMEPLAY_RECIPES_HPP
#define GAMEPLAY_RECIPES_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

#include "utility/optional.hpp"
#include "utility/ranges.hpp"

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

	class SkillAmount
	{
	public:
		std::string name;
		double amount;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &SkillAmount::name),
				std::make_pair(utility::string_view("amount"), &SkillAmount::amount)
				);
		}
	};

	class Recipe
	{
	public:
		std::string name;
		int durability;
		utility::optional<int> time;
		utility::optional<int> amount;
		std::vector<Ingredient> ingredients;
		std::vector<SkillAmount> skill_amounts;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &Recipe::name),
				std::make_pair(utility::string_view("durability"), &Recipe::durability),
				std::make_pair(utility::string_view("time"), &Recipe::time),
				std::make_pair(utility::string_view("amount"), &Recipe::amount),
				std::make_pair(utility::string_view("ingredients"), &Recipe::ingredients),
				std::make_pair(utility::string_view("skills"), &Recipe::skill_amounts)
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
		std::ptrdiff_t find(const std::string & name) const
		{
			for (std::ptrdiff_t i : ranges::index_sequence_for(recipes))
			{
				if (recipes[i].name == name)
					return i;
			}
			return -1;
		}
		const Recipe & get(std::ptrdiff_t i) const { return recipes[i]; }
		std::ptrdiff_t index(const Recipe & recipe) const { return &recipe - recipes.data(); }
		std::size_t size() const { return recipes.size(); }

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
