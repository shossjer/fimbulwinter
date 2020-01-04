
#ifndef GAMEPLAY_ROLES_HPP
#define GAMEPLAY_ROLES_HPP

#include "core/serialization.hpp"

#include "engine/Asset.hpp"

#include "utility/optional.hpp"
#include "utility/ranges.hpp"

#include <string>
#include <vector>

namespace gameplay
{
	class SkillWeight
	{
	public:
		std::string name;
		double weight;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &SkillWeight::name),
				std::make_pair(utility::string_view("weight"), &SkillWeight::weight)
				);
		}
	};

	class Role
	{
	public:
		std::string name;
		int default_weight;
		std::vector<SkillWeight> skill_weights;

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("name"), &Role::name),
				std::make_pair(utility::string_view("default"), &Role::default_weight),
				std::make_pair(utility::string_view("skills"), &Role::skill_weights)
				);
		}
	};

	class Roles
	{
	private:
		std::vector<Role> roles;

	public:
		Roles() = default;
		Roles(const Roles &) = delete;
		Roles & operator = (const Roles &) = delete;

	public:
		std::ptrdiff_t find(const std::string & name) const
		{
			for (std::ptrdiff_t i : ranges::index_sequence_for(roles))
			{
				if (roles[i].name == name)
					return i;
			}
			return -1;
		}
		const Role & get(std::ptrdiff_t i) const { return roles[i]; }
		std::ptrdiff_t index(const Role & role) const { return &role - roles.data(); }
		std::size_t size() const { return roles.size(); }

	public:
		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("classes"), &Roles::roles)
				);
		}
	};
}

#endif /* GAMEPLAY_ROLES_HPP */
