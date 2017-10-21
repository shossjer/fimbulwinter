
// should not be included outside gui namespace.

#ifndef ENGINE_GUI2_RENDER_HPP
#define ENGINE_GUI2_RENDER_HPP

#include "view.hpp"

namespace engine
{
	namespace gui2
	{
		void renderer_add(View::Color & content)
		{
			// TODO:
		}
		void renderer_move(View::Color & content)
		{
			// TODO:
		}
		void renderer_update(View::Color & content)
		{
			// TODO:
		}
		void renderer_add(View::Text & content)
		{
			// TODO:
		}
		void renderer_move(View::Text & content)
		{
			// TODO:
		}
		void renderer_update(View::Text & content)
		{
			// TODO:
		}

		template<typename T>
		void renderer_refresh(T & content)
		{
			if (true)
			{
				renderer_add(content);
			}
			else if (false)
			{
				// TODO: check if 'just' move
				renderer_update(content);
			}
			else
			{
				// send remove
			}
		}
	}
}

#endif
