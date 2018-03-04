
#ifndef GAMEPLAY_SKILLS_HPP
#define GAMEPLAY_SKILLS_HPP

#include "core/serialize.hpp"

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
		template <typename S>
		friend void serialize_class(S & s, Skill & x)
		{
			using core::serialize;

			serialize(s, "name", x.name);
			serialize(s, "type", x.type);
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
		template <typename S>
		friend void serialize_class(S & s, Skills & x)
		{
			using core::serialize;

			serialize(s, "skills", x.skills);
		}
	};
}

#endif /* GAMEPLAY_SKILLS_HPP */
