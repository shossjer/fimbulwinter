
#ifndef GAMEPLAY_CHARACTERS_HPP
#define GAMEPLAY_CHARACTERS_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>

namespace gameplay
{
namespace characters
{
	enum struct Command
	{
		LEFT_DOWN,
		LEFT_UP,
		RIGHT_DOWN,
		RIGHT_UP,
		UP_DOWN,
		UP_UP,
		DOWN_DOWN,
		DOWN_UP
	};

	void update();

	void create(const engine::Entity id);

	void remove(const engine::Entity id);

	void post_command(engine::Entity id, Command command);

	void post_add_camera(engine::Entity id, engine::Entity target);

	void post_animation_finish(engine::Entity id);
}
}


#endif /* GAMEPLAY_CHARACTERS_HPP */
