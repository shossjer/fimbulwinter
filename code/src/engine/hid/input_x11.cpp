
#include <config.h>

#if WINDOW_USE_X11

#include "input.hpp"

#include "engine/debug.hpp"

#include "utility/unicode.hpp"

#include <X11/X.h>
#include <X11/Xlib.h>

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
		void key_character(XKeyEvent & event, utility::code_point cp)
		{
			input.setKeyCharacter(0, engine::hid::Input::Button::INVALID, cp);
			dispatch(input);
		}

		void button_press(XButtonEvent & event)
		{

			input.setButtonDown(0, engine::hid::Input::Button::INVALID, event.x, event.y);
			dispatch(input);
		}
		void button_release(XButtonEvent & event)
		{

			input.setButtonUp(0, engine::hid::Input::Button::INVALID, event.x, event.y);
			dispatch(input);
		}
		void key_press(XKeyEvent & event)
		{
			input.setButtonDown(0, engine::hid::Input::Button::INVALID, event.x, event.y);
			dispatch(input);
		}
		void key_release(XKeyEvent & event)
		{
			input.setButtonUp(0, engine::hid::Input::Button::INVALID, event.x, event.y);
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
