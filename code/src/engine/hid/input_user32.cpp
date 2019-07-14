
#include <config.h>

#if WINDOW_USE_USER32

#include "input.hpp"

#include <Windows.h>

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
		void key_down(BYTE virtual_key,
		              BYTE scan_code,
		              LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyDown(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
		void key_up(BYTE virtual_key,
		            BYTE scan_code,
		            LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyUp(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
		void lbutton_down(LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::MOUSE_LEFT, 0, 0);
			dispatch(input);
		}
		void lbutton_up(LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::MOUSE_LEFT, 0, 0);
			dispatch(input);
		}
		void mbutton_down(LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::MOUSE_MIDDLE, 0, 0);
			dispatch(input);
		}
		void mbutton_up(LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::MOUSE_MIDDLE, 0, 0);
			dispatch(input);
		}
		void rbutton_down(LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::MOUSE_RIGHT, 0, 0);
			dispatch(input);
		}
		void rbutton_up(LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::MOUSE_RIGHT, 0, 0);
			dispatch(input);
		}
		void mouse_move(const int_fast16_t x,
		                const int_fast16_t y,
		                LONG time)
		{
			input.setCursorAbsolute(0, x, y);
			dispatch(input);
		}
		void mouse_wheel(const int_fast16_t delta,
		                 LONG time)
		{
			// TODO:
		}

		void syskey_down(BYTE virtual_key,
		                 BYTE scan_code,
		                 LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyDown(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
		void syskey_up(BYTE virtual_key,
		               BYTE scan_code,
		               LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
			input.setKeyUp(0, engine::hid::Input::Button::KEY_0, utility::code_point(nullptr));
			dispatch(input);
		}
	}
}

#endif /* WINDOW_USE_USER32 */
