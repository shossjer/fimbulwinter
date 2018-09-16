
#ifndef GAMEPLAY_SKILLS_HPP
#define GAMEPLAY_SKILLS_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

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
		const Skill & get(int i) const { return skills[i]; }
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
