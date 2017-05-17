
#ifndef GAMEPLAY_GAMESTATE_HPP
#define GAMEPLAY_GAMESTATE_HPP

#include <engine/Entity.hpp>

namespace gameplay
{
namespace gamestate
{
	void create();

	void destroy();

	void update();

	enum class Command : int
	{
		CONTEXT_CHANGED,
		MOVE_DOWN_DOWN,
		MOVE_DOWN_UP,
		MOVE_LEFT_DOWN,
		MOVE_LEFT_UP,
		MOVE_RIGHT_DOWN,
		MOVE_RIGHT_UP,
		MOVE_UP_DOWN,
		MOVE_UP_UP,
		RENDER_SELECT,
		ROLL_LEFT_DOWN,
		ROLL_LEFT_UP,
		ROLL_RIGHT_DOWN,
		ROLL_RIGHT_UP,
		TURN_DOWN_DOWN,
		TURN_DOWN_UP,
		TURN_LEFT_DOWN,
		TURN_LEFT_UP,
		TURN_RIGHT_DOWN,
		TURN_RIGHT_UP,
		TURN_UP_DOWN,
		TURN_UP_UP
	};

	void post_command(engine::Entity entity, Command command);
	void post_command(engine::Entity entity, Command command, engine::Entity arg1);
}
}

#endif /* GAMEPLAY_GAMESTATE_HPP */
