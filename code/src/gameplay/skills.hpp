
#ifndef GAMEPLAY_SKILLS_HPP
#define GAMEPLAY_SKILLS_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

#include "utility/ranges.hpp"

#include <string>
#include <vector>

namespace gameplay
{
	class Skill
	{
	public:
		std::string name;
		std::string type;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &Skill::name),
				std::make_pair(utility::string_view("type"), &Skill::type)
				);
		}
	};

	class Skills
	{
	private:
		std::vector<Skill> skills;

	public:
		Skills() = default;
		Skills(const Skills &) = delete;
		Skills & operator = (const Skills &) = delete;

	public:
		int find(const std::string & name) const
		{
			for (int i : ranges::index_sequence_for(skills))
			{
				if (skills[i].name == name)
					return i;
			}
			return -1;
		}
		const Skill & get(int i) const { return skills[i]; }
		int index(const Skill & skill) const { return static_cast<int>(&skill - skills.data()); }
		int size() const { return skills.size(); }

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("skills"), &Skills::skills)
				);
		}
	};
}

#endif /* GAMEPLAY_SKILLS_HPP */
