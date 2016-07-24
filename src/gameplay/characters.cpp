
#include "characters.hpp"

#include <core/debug.hpp>

#include <unordered_map>

namespace
{
	std::unordered_map<engine::Entity, ::gameplay::CharacterState> items;
}

namespace gameplay
{
namespace characters
{

	void create(const engine::Entity id)
	{
		items.emplace(id, ::gameplay::CharacterState{ });
	}

	void remove(const engine::Entity id)
	{
		items.erase(id);
	}

	CharacterState & get(const engine::Entity id)
	{
		return items.at(id);
	}
}
}
