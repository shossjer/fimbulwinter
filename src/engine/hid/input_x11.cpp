
#include <config.h>

#if WINDOW_USE_X11

#include "input.hpp"

#include <X11/X.h>

namespace engine
{
	namespace hid
	{
		extern void dispatch(const Input & input);
	}
}

namespace
{
	engine::hid::Input input;
}

namespace engine
{
	namespace hid
	{
		void key_press(const unsigned int keycode,
		               const unsigned int state,
		               const ::Time time)
		{
			input.setDown(keycode, state);

			dispatch(input);
		}
		void key_release(const unsigned int keycode,
		                 const unsigned int state,
		                 const ::Time time)
		{
			input.setUp(keycode, state);

			dispatch(input);
		}
		void motion_notify(const int x,
		                   const int y,
		                   const ::Time time)
		{
			input.setMove(x - input.getCursor().x,
			              y - input.getCursor().y);
			input.setCursor(x, y);

			dispatch(input);
		}
	}
}

#endif /* WINDOW_USE_X11 */
