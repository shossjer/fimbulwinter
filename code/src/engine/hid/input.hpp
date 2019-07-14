
#ifndef ENGINE_HID_INPUT_HPP
#define ENGINE_HID_INPUT_HPP

#include "engine/debug.hpp"

#include "utility/unicode.hpp"

#include <cstdint>
#include <cstddef>

namespace engine
{
	namespace hid
	{
		union Input
		{
		public:
			enum class State : int8_t
			{
				BUTTON_DOWN,
				BUTTON_UP,
				CURSOR_ABSOLUTE,
				KEY_DOWN,
				KEY_UP,
				COUNT,
			};

			using Player = int8_t;

			enum class Button : int16_t
			{
				MOUSE_BACK = 0,
				MOUSE_EXTRA,
				MOUSE_FORWARD,
				MOUSE_LEFT,
				MOUSE_MIDDLE,
				MOUSE_RIGHT,
				MOUSE_SIDE,
				MOUSE_TASK,
				KEY_0 = 32,
				KEY_1,
				KEY_2,
				KEY_3,
				KEY_4,
				KEY_5,
				KEY_6,
				KEY_7,
				KEY_8,
				KEY_9,
				KEY_A,
				KEY_B,
				KEY_C,
				KEY_D,
				KEY_E,
				KEY_F,
				KEY_G,
				KEY_H,
				KEY_I,
				KEY_J,
				KEY_K,
				KEY_L,
				KEY_M,
				KEY_N,
				KEY_O,
				KEY_P,
				KEY_Q,
				KEY_R,
				KEY_S,
				KEY_T,
				KEY_U,
				KEY_V,
				KEY_W,
				KEY_X,
				KEY_Y,
				KEY_Z,
				KEY_102ND,
				KEY_APOSTROPHE,
				KEY_BACKSLASH,
				KEY_BACKSPACE,
				KEY_CAPSLOCK,
				KEY_COMMA,
				KEY_DELETE,
				KEY_DOT,
				KEY_DOWN,
				KEY_END,
				KEY_ENTER,
				KEY_ESC,
				KEY_EQUAL,
				KEY_F1,
				KEY_F2,
				KEY_F3,
				KEY_F4,
				KEY_F5,
				KEY_F6,
				KEY_F7,
				KEY_F8,
				KEY_F9,
				KEY_F10,
				KEY_F11,
				KEY_F12,
				KEY_F13,
				KEY_F14,
				KEY_F15,
				KEY_F16,
				KEY_F17,
				KEY_F18,
				KEY_F19,
				KEY_F20,
				KEY_F21,
				KEY_F22,
				KEY_F23,
				KEY_F24,
				KEY_GRAVE,
				KEY_HOME,
				KEY_INSERT,
				KEY_KP0,
				KEY_KP1,
				KEY_KP2,
				KEY_KP3,
				KEY_KP4,
				KEY_KP5,
				KEY_KP6,
				KEY_KP7,
				KEY_KP8,
				KEY_KP9,
				KEY_KPASTERISK,
				KEY_KPMINUS,
				KEY_KPPLUS,
				KEY_KPSLASH,
				KEY_LEFT,
				KEY_LEFTALT,
				KEY_LEFTCTRL,
				KEY_LEFTBRACE,
				KEY_LEFTMETA,
				KEY_LEFTSHIFT,
				KEY_MINUS,
				KEY_PAGEDOWN,
				KEY_PAGEUP,
				KEY_PAUSE,
				KEY_RIGHT,
				KEY_RIGHTALT,
				KEY_RIGHTBRACE,
				KEY_RIGHTCTRL,
				KEY_RIGHTMETA,
				KEY_RIGHTSHIFT,
				KEY_SEMICOLON,
				KEY_SLASH,
				KEY_SPACE,
				KEY_TAB,
				KEY_UP,
			};

			struct Position
			{
				int16_t x, y;
			};

		private:
			struct CommonHeader
			{
				State state;
				Player player;
			} common_header;
			static_assert(std::is_trivial<CommonHeader>::value, "");

			struct ButtonDown
			{
				State state;
				Player player;
				Button code;
				Position position;
			} button_down;
			static_assert(std::is_trivial<ButtonDown>::value, "");

			struct ButtonUp
			{
				State state;
				Player player;
				Button code;
				Position position;
			} button_up;
			static_assert(std::is_trivial<ButtonUp>::value, "");

			struct CursorAbsolute
			{
				State state;
				Player player;
				uint16_t unused;
				Position position;
			} cursor_absolute;
			static_assert(std::is_trivial<CursorAbsolute>::value, "");

			struct KeyDown
			{
				State state;
				Player player;
				Button code;
				utility::code_point unicode;
			} key_down;
			static_assert(std::is_trivial<KeyDown>::value, "");

			struct KeyUp
			{
				State state;
				Player player;
				Button code;
				utility::code_point unicode;
			} key_up;
			static_assert(std::is_trivial<KeyUp>::value, "");


		public:
			/**
			 * \note Valid iff state is `BUTTON_DOWN` or `BUTTON_UP`.
			 */
			Button getButton() const
			{
				switch (common_header.state)
				{
				case State::BUTTON_DOWN: return button_down.code;
				case State::BUTTON_UP: return button_up.code;
				default: debug_unreachable();
				}
			}

			int getPlayer() const { return common_header.player; }

			/**
			 * \note Valid iff state is `BUTTON_DOWN`, `BUTTON_UP`, or `CURSOR_ABSOLUTE`.
			 */
			Position getPosition() const
			{
				switch (common_header.state)
				{
				case State::BUTTON_DOWN: return button_down.position;
				case State::BUTTON_UP: return button_up.position;
				case State::CURSOR_ABSOLUTE: return cursor_absolute.position;
				default: debug_unreachable();
				}
			}

			State getState() const { return common_header.state; }

			/**
			 * \note Valid iff state is `KEY_DOWN` or `KEY_UP`.
			 */
			utility::code_point getUnicode() const
			{
				switch (common_header.state)
				{
				case State::KEY_DOWN: return key_down.unicode;
				case State::KEY_UP: return key_down.unicode;
				default: debug_unreachable();
				}
			}

			void setButtonDown(int_fast8_t player, Button code, int_fast16_t x, int_fast16_t y)
			{
				button_down.state = State::BUTTON_DOWN;
				button_down.player = player;
				button_down.code = code;
				button_down.position.x = x;
				button_down.position.y = y;
			}

			void setButtonUp(int_fast8_t player, Button code, int_fast16_t x, int_fast16_t y)
			{
				button_up.state = State::BUTTON_UP;
				button_up.player = player;
				button_up.code = code;
				button_up.position.x = x;
				button_up.position.y = y;
			}

			void setCursorAbsolute(int_fast8_t player, int_fast16_t x, int_fast16_t y)
			{
				cursor_absolute.state = State::CURSOR_ABSOLUTE;
				cursor_absolute.player = player;
				cursor_absolute.position.x = x;
				cursor_absolute.position.y = y;
			}

			void setKeyDown(int_fast8_t player, Button code, utility::code_point unicode)
			{
				key_down.state = State::KEY_DOWN;
				key_down.player = player;
				key_down.code = code;
				key_down.unicode = unicode;
			}

			void setKeyUp(int_fast8_t player, Button code, utility::code_point unicode)
			{
				key_up.state = State::KEY_UP;
				key_up.player = player;
				key_up.code = code;
				key_up.unicode = unicode;
			}
		};
		static_assert(sizeof(Input) == 8, "This is not a hard requirement but it would be nice to know if it grows.");
	}
}

#endif /* ENGINE_HID_INPUT_HPP */
