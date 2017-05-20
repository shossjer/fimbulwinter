
#ifndef GAMEPLAY_GAMESTATE_HPP
#define GAMEPLAY_GAMESTATE_HPP

#include <engine/Command.hpp>
#include <engine/Entity.hpp>

namespace gameplay
{
namespace gamestate
{
	void create();

	void destroy();

	void update();

	void post_command(engine::Entity entity, engine::Command command);
	void post_command(engine::Entity entity, engine::Command command, engine::Entity arg1);

	enum class WorkstationType
	{
		BENCH,
		OVEN
	};

	void post_add_workstation(engine::Entity entity, WorkstationType type);

	void post_add_worker(engine::Entity entity);
}
}

#endif /* GAMEPLAY_GAMESTATE_HPP */
