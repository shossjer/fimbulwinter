
#ifndef GAMEPLAY_CHARACTERS_HPP
#define GAMEPLAY_CHARACTERS_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>

namespace gameplay
{
namespace characters
{
	enum MovementState
	{
		NONE,	// no active movement (can still be falling etc)

		LEFT,
		LEFT_UP,
		UP,
		RIGHT_UP,
		RIGHT,
		RIGHT_DOWN,
		DOWN,
		LEFT_DOWN
	};

	void update();

	void create(const engine::Entity id);

	void remove(const engine::Entity id);
	/**
	 *	Entity grounded/falling state- or its ground normal- has changed
	 */
	void postGrounded(const engine::Entity id, const core::maths::Vector3f groundNormal);
	/**
	 *	Entity state is changed to falling
	 */
	void postFalling(const engine::Entity id);
	/**
	 *	Entity movement state has changed
	 */
	void postMovement(const engine::Entity id, const MovementState state);
}
}


#endif /* GAMEPLAY_CHARACTERS_HPP */
