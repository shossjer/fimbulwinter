
#ifndef GAMEPLAY_CHARACTERS_HPP
#define GAMEPLAY_CHARACTERS_HPP

#include <engine/Entity.hpp>

#include <gameplay/CharacterState.hpp>

namespace gameplay
{
namespace characters
{
	void create(const engine::Entity id);

	void remove(const engine::Entity id);

	CharacterState & get(const engine::Entity id);
}
}


#endif /* GAMEPLAY_CHARACTERS_HPP */
