
#ifndef ENGINE_COMMAND_HPP
#define ENGINE_COMMAND_HPP

namespace engine
{
	enum class Command : int
	{
		BUTTON_DOWN_ACTIVE,
		BUTTON_UP_ACTIVE,
		BUTTON_DOWN_INACTIVE,
		BUTTON_UP_INACTIVE,
		CONTEXT_CHANGED,
		ELEVATE_DOWN_DOWN,
		ELEVATE_DOWN_UP,
		ELEVATE_UP_DOWN,
		ELEVATE_UP_UP,
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
}

#endif /* ENGINE_COMMAND_HPP */
