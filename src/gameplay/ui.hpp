
#ifndef GAMEPLAY_UI_HPP
#define GAMEPLAY_UI_HPP

#include <engine/Entity.hpp>

namespace gameplay
{
	namespace ui
	{
		void post_add_player(engine::Entity entity);
		void post_remove(engine::Entity entity);

		struct coords_t
		{
			short x;
			short y;
		};

		/**
		 * \warning NOT thread safe.
		 */
		coords_t mouseCoords();
		/**
		 * \warning NOT thread safe.
		 */
		coords_t mouseDelta();
	}
}

#endif /* GAMEPLAY_UI_HPP */
