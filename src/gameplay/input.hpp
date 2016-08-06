
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
		struct coords_t
		{
			signed short x;
			signed short y;
		};
		/**
		 *	Mouse Coordinates of the frame
		 */
		coords_t mouseCoords();
		/**
		 *	Mouse movement of the frame
		 */
		coords_t mouseDelta();

		void updateInput();

		void updateCamera();

		void create();
		void destroy();
	}
}

#endif /* GAMEPLAY_PLAYER_HPP */
