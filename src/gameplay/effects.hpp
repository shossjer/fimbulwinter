
#ifndef GAMEPLAY_EFFECTS_HPP
#define GAMEPLAY_EFFECTS_HPP

#include <engine/Entity.hpp>

namespace gameplay
{
namespace effects
{
	typedef unsigned int Id;

	void update();

	enum class Type
	{
		PLAYER_GRAVITY
	};

	engine::Entity create(const Type type, const engine::Entity callerId);

	void remove(const engine::Entity id);
}
}

#endif // GAMEPLAY_EFFECTS_HPP
