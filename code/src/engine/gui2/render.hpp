
// should not be included outside gui namespace.

#ifndef ENGINE_GUI2_RENDER_HPP
#define ENGINE_GUI2_RENDER_HPP

#include "view.hpp"

namespace engine
{
	namespace gui2
	{
		struct ViewRenderer
		{
		private:

			static void add(View & view, View::Color & content);

			static void add(View & view, View::Text & content);

			static void update(View & view, View::Color & content);

			static void update(View & view, View::Text & content);

		public:

			template<typename T>
			static constexpr void refresh(View & view, T & content)
			{
				if (view.status.rendered)
				{
					// TODO: check if 'just' move
					update(view, content);
				}
				else
				{
					add(view, content);
				}
			}

			static void remove(View & view);
		};
	}
}

#endif
