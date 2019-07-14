
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
		void button_press(const unsigned int buttoncode,
		                  const unsigned int state,
		                  const ::Time time)
		{

			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
		}
		void button_release(const unsigned int buttoncode,
		                    const unsigned int state,
		                    const ::Time time)
		{

			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
		}
		void key_press(const unsigned int keycode,
		               const unsigned int state,
		               const ::Time time)
		{

			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyDown(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
		void key_release(const unsigned int keycode,
		                 const unsigned int state,
		                 const ::Time time)
		{

			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyUp(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
		void motion_notify(const int x,
		                   const int y,
		                   const ::Time time)
		{
			input.setCursorAbsolute(0, x, y);
			dispatch(input);
		}
	}
}

#endif /* WINDOW_USE_X11 */
