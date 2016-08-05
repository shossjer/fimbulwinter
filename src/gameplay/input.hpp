
#ifndef GAMEPLAY_PLAYER_HPP
#define GAMEPLAY_PLAYER_HPP

#include <engine/Entity.hpp>

namespace gameplay
{
	namespace player
	{
		engine::Entity get();

		void set(const engine::Entity id);
	}

	namespace input
	{
		void updateInput();

		void updateCamera();

		void create();
		void destroy();
	}
}

#endif /* GAMEPLAY_PLAYER_HPP */
