
#ifndef ENGINE_HID_INPUT_HPP
#define ENGINE_HID_INPUT_HPP

#include "core/serialization.hpp"

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
				AXIS_TILT,
				AXIS_TRIGGER,
				BUTTON_DOWN,
				BUTTON_UP,
				CURSOR_MOVE,
				KEY_CHARACTER,
			};

			static constexpr int state_count = static_cast<int>(State::KEY_CHARACTER) + 1;

			using Player = int8_t;

			static constexpr int max_player = 127;

			enum class Axis : int8_t
			{
				INVALID = 0, // should never be used
				MOUSE_MOVE = 1, // not really an axis but hey :shrug:
				TILT_DPAD_X = 8,
				TILT_DPAD_Y,
				TILT_STICKL_X,
				TILT_STICKL_Y,
				TILT_STICKR_X,
				TILT_STICKR_Y,
				TRIGGER_TL2 = 16,
				TRIGGER_TR2,
			};

			static constexpr int axis_count = static_cast<int>(Axis::TRIGGER_TR2) + 1;

			friend constexpr auto serialization(utility::in_place_type_t<Axis>)
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("invalid"), Axis::INVALID),
					std::make_pair(utility::string_view("mouse-move"), Axis::MOUSE_MOVE),
					std::make_pair(utility::string_view("tilt-dpad-x"), Axis::TILT_DPAD_X),
					std::make_pair(utility::string_view("tilt-dpad-y"), Axis::TILT_DPAD_Y),
					std::make_pair(utility::string_view("tilt-stickl-x"), Axis::TILT_STICKL_X),
					std::make_pair(utility::string_view("tilt-stickl-y"), Axis::TILT_STICKL_Y),
					std::make_pair(utility::string_view("tilt-stickr-x"), Axis::TILT_STICKR_X),
					std::make_pair(utility::string_view("tilt-stickr-y"), Axis::TILT_STICKR_Y),
					std::make_pair(utility::string_view("trigger-tl2"), Axis::TRIGGER_TL2),
					std::make_pair(utility::string_view("trigger-tr2"), Axis::TRIGGER_TR2)
					);
			}

			// \note Keep in sync with:
			// - `button_to_code` in `input_x11.cpp`
			enum class Button : int16_t
			{
				INVALID = 0, // should never be used
				MOUSE_EXTRA = 1,
				MOUSE_LEFT,
				MOUSE_MIDDLE,
				MOUSE_RIGHT,
				MOUSE_SIDE,
				GAMEPAD_EAST = 8,
				GAMEPAD_NORTH,
				GAMEPAD_SOUTH,
				GAMEPAD_WEST,
				GAMEPAD_THUMBL,
				GAMEPAD_THUMBR,
				GAMEPAD_TL,
				GAMEPAD_TL2,
				GAMEPAD_TR,
				GAMEPAD_TR2,
				GAMEPAD_MODE,
				GAMEPAD_SELECT,
				GAMEPAD_START,
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
				KEY_BREAK,
				KEY_CAPSLOCK,
				KEY_CLEAR,
				KEY_COMMA,
				KEY_COMPOSE,
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
				KEY_KPDOT,
				KEY_KPENTER,
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
				KEY_NUMLOCK,
				KEY_PAGEDOWN,
				KEY_PAGEUP,
				KEY_PAUSE,
				KEY_PRINTSCREEN,
				KEY_RIGHT,
				KEY_RIGHTALT,
				KEY_RIGHTBRACE,
				KEY_RIGHTCTRL,
				KEY_RIGHTMETA,
				KEY_RIGHTSHIFT,
				KEY_SCROLLLOCK,
				KEY_SEMICOLON,
				KEY_SLASH,
				KEY_SPACE,
				KEY_SYSRQ,
				KEY_TAB,
				KEY_UP,
			};

			static constexpr int button_count = static_cast<int>(Button::KEY_UP) + 1;

			friend constexpr auto serialization(utility::in_place_type_t<Button>)
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_view("invalid"), Button::INVALID),
					std::make_pair(utility::string_view("mouse-extra"), Button::MOUSE_EXTRA),
					std::make_pair(utility::string_view("mouse-left"), Button::MOUSE_LEFT),
					std::make_pair(utility::string_view("mouse-middle"), Button::MOUSE_MIDDLE),
					std::make_pair(utility::string_view("mouse-right"), Button::MOUSE_RIGHT),
					std::make_pair(utility::string_view("mouse-side"), Button::MOUSE_SIDE),
					std::make_pair(utility::string_view("gamepad-east"), Button::GAMEPAD_EAST),
					std::make_pair(utility::string_view("gamepad-north"), Button::GAMEPAD_NORTH),
					std::make_pair(utility::string_view("gamepad-south"), Button::GAMEPAD_SOUTH),
					std::make_pair(utility::string_view("gamepad-west"), Button::GAMEPAD_WEST),
					std::make_pair(utility::string_view("gamepad-thumbl"), Button::GAMEPAD_THUMBL),
					std::make_pair(utility::string_view("gamepad-thumbr"), Button::GAMEPAD_THUMBR),
					std::make_pair(utility::string_view("gamepad-tl"), Button::GAMEPAD_TL),
					std::make_pair(utility::string_view("gamepad-tl2"), Button::GAMEPAD_TL2),
					std::make_pair(utility::string_view("gamepad-tr"), Button::GAMEPAD_TR),
					std::make_pair(utility::string_view("gamepad-tr2"), Button::GAMEPAD_TR2),
					std::make_pair(utility::string_view("gamepad-mode"), Button::GAMEPAD_MODE),
					std::make_pair(utility::string_view("gamepad-select"), Button::GAMEPAD_SELECT),
					std::make_pair(utility::string_view("gamepad-start"), Button::GAMEPAD_START),
					std::make_pair(utility::string_view("key-0"), Button::KEY_0),
					std::make_pair(utility::string_view("key-1"), Button::KEY_1),
					std::make_pair(utility::string_view("key-2"), Button::KEY_2),
					std::make_pair(utility::string_view("key-3"), Button::KEY_3),
					std::make_pair(utility::string_view("key-4"), Button::KEY_4),
					std::make_pair(utility::string_view("key-5"), Button::KEY_5),
					std::make_pair(utility::string_view("key-6"), Button::KEY_6),
					std::make_pair(utility::string_view("key-7"), Button::KEY_7),
					std::make_pair(utility::string_view("key-8"), Button::KEY_8),
					std::make_pair(utility::string_view("key-9"), Button::KEY_9),
					std::make_pair(utility::string_view("key-a"), Button::KEY_A),
					std::make_pair(utility::string_view("key-b"), Button::KEY_B),
					std::make_pair(utility::string_view("key-c"), Button::KEY_C),
					std::make_pair(utility::string_view("key-d"), Button::KEY_D),
					std::make_pair(utility::string_view("key-e"), Button::KEY_E),
					std::make_pair(utility::string_view("key-f"), Button::KEY_F),
					std::make_pair(utility::string_view("key-g"), Button::KEY_G),
					std::make_pair(utility::string_view("key-h"), Button::KEY_H),
					std::make_pair(utility::string_view("key-i"), Button::KEY_I),
					std::make_pair(utility::string_view("key-j"), Button::KEY_J),
					std::make_pair(utility::string_view("key-k"), Button::KEY_K),
					std::make_pair(utility::string_view("key-l"), Button::KEY_L),
					std::make_pair(utility::string_view("key-m"), Button::KEY_M),
					std::make_pair(utility::string_view("key-n"), Button::KEY_N),
					std::make_pair(utility::string_view("key-o"), Button::KEY_O),
					std::make_pair(utility::string_view("key-p"), Button::KEY_P),
					std::make_pair(utility::string_view("key-q"), Button::KEY_Q),
					std::make_pair(utility::string_view("key-r"), Button::KEY_R),
					std::make_pair(utility::string_view("key-s"), Button::KEY_S),
					std::make_pair(utility::string_view("key-t"), Button::KEY_T),
					std::make_pair(utility::string_view("key-u"), Button::KEY_U),
					std::make_pair(utility::string_view("key-v"), Button::KEY_V),
					std::make_pair(utility::string_view("key-w"), Button::KEY_W),
					std::make_pair(utility::string_view("key-x"), Button::KEY_X),
					std::make_pair(utility::string_view("key-y"), Button::KEY_Y),
					std::make_pair(utility::string_view("key-z"), Button::KEY_Z),
					std::make_pair(utility::string_view("key-102nd"), Button::KEY_102ND),
					std::make_pair(utility::string_view("key-apostrophe"), Button::KEY_APOSTROPHE),
					std::make_pair(utility::string_view("key-backslash"), Button::KEY_BACKSLASH),
					std::make_pair(utility::string_view("key-backspace"), Button::KEY_BACKSPACE),
					std::make_pair(utility::string_view("key-break"), Button::KEY_BREAK),
					std::make_pair(utility::string_view("key-capslock"), Button::KEY_CAPSLOCK),
					std::make_pair(utility::string_view("key-clear"), Button::KEY_CLEAR),
					std::make_pair(utility::string_view("key-comma"), Button::KEY_COMMA),
					std::make_pair(utility::string_view("key-compose"), Button::KEY_COMPOSE),
					std::make_pair(utility::string_view("key-delete"), Button::KEY_DELETE),
					std::make_pair(utility::string_view("key-dot"), Button::KEY_DOT),
					std::make_pair(utility::string_view("key-down"), Button::KEY_DOWN),
					std::make_pair(utility::string_view("key-end"), Button::KEY_END),
					std::make_pair(utility::string_view("key-enter"), Button::KEY_ENTER),
					std::make_pair(utility::string_view("key-equal"), Button::KEY_EQUAL),
					std::make_pair(utility::string_view("key-esc"), Button::KEY_ESC),
					std::make_pair(utility::string_view("key-f1"), Button::KEY_F1),
					std::make_pair(utility::string_view("key-f2"), Button::KEY_F2),
					std::make_pair(utility::string_view("key-f3"), Button::KEY_F3),
					std::make_pair(utility::string_view("key-f4"), Button::KEY_F4),
					std::make_pair(utility::string_view("key-f5"), Button::KEY_F5),
					std::make_pair(utility::string_view("key-f6"), Button::KEY_F6),
					std::make_pair(utility::string_view("key-f7"), Button::KEY_F7),
					std::make_pair(utility::string_view("key-f8"), Button::KEY_F8),
					std::make_pair(utility::string_view("key-f9"), Button::KEY_F9),
					std::make_pair(utility::string_view("key-f10"), Button::KEY_F10),
					std::make_pair(utility::string_view("key-f11"), Button::KEY_F11),
					std::make_pair(utility::string_view("key-f12"), Button::KEY_F12),
					std::make_pair(utility::string_view("key-f13"), Button::KEY_F13),
					std::make_pair(utility::string_view("key-f14"), Button::KEY_F14),
					std::make_pair(utility::string_view("key-f15"), Button::KEY_F15),
					std::make_pair(utility::string_view("key-f16"), Button::KEY_F16),
					std::make_pair(utility::string_view("key-f17"), Button::KEY_F17),
					std::make_pair(utility::string_view("key-f18"), Button::KEY_F18),
					std::make_pair(utility::string_view("key-f19"), Button::KEY_F19),
					std::make_pair(utility::string_view("key-f20"), Button::KEY_F20),
					std::make_pair(utility::string_view("key-f21"), Button::KEY_F21),
					std::make_pair(utility::string_view("key-f22"), Button::KEY_F22),
					std::make_pair(utility::string_view("key-f23"), Button::KEY_F23),
					std::make_pair(utility::string_view("key-f24"), Button::KEY_F24),
					std::make_pair(utility::string_view("key-grave"), Button::KEY_GRAVE),
					std::make_pair(utility::string_view("key-home"), Button::KEY_HOME),
					std::make_pair(utility::string_view("key-insert"), Button::KEY_INSERT),
					std::make_pair(utility::string_view("key-kp0"), Button::KEY_KP0),
					std::make_pair(utility::string_view("key-kp1"), Button::KEY_KP1),
					std::make_pair(utility::string_view("key-kp2"), Button::KEY_KP2),
					std::make_pair(utility::string_view("key-kp3"), Button::KEY_KP3),
					std::make_pair(utility::string_view("key-kp4"), Button::KEY_KP4),
					std::make_pair(utility::string_view("key-kp5"), Button::KEY_KP5),
					std::make_pair(utility::string_view("key-kp6"), Button::KEY_KP6),
					std::make_pair(utility::string_view("key-kp7"), Button::KEY_KP7),
					std::make_pair(utility::string_view("key-kp8"), Button::KEY_KP8),
					std::make_pair(utility::string_view("key-kp9"), Button::KEY_KP9),
					std::make_pair(utility::string_view("key-kpasterisk"), Button::KEY_KPASTERISK),
					std::make_pair(utility::string_view("key-kpdot"), Button::KEY_KPDOT),
					std::make_pair(utility::string_view("key-kpenter"), Button::KEY_KPENTER),
					std::make_pair(utility::string_view("key-kpminus"), Button::KEY_KPMINUS),
					std::make_pair(utility::string_view("key-kpplus"), Button::KEY_KPPLUS),
					std::make_pair(utility::string_view("key-kpslash"), Button::KEY_KPSLASH),
					std::make_pair(utility::string_view("key-left"), Button::KEY_LEFT),
					std::make_pair(utility::string_view("key-leftalt"), Button::KEY_LEFTALT),
					std::make_pair(utility::string_view("key-leftbrace"), Button::KEY_LEFTBRACE),
					std::make_pair(utility::string_view("key-leftctrl"), Button::KEY_LEFTCTRL),
					std::make_pair(utility::string_view("key-leftmeta"), Button::KEY_LEFTMETA),
					std::make_pair(utility::string_view("key-leftshift"), Button::KEY_LEFTSHIFT),
					std::make_pair(utility::string_view("key-minus"), Button::KEY_MINUS),
					std::make_pair(utility::string_view("key-numlock"), Button::KEY_NUMLOCK),
					std::make_pair(utility::string_view("key-pagedown"), Button::KEY_PAGEDOWN),
					std::make_pair(utility::string_view("key-pageup"), Button::KEY_PAGEUP),
					std::make_pair(utility::string_view("key-pause"), Button::KEY_PAUSE),
					std::make_pair(utility::string_view("key-printscreen"), Button::KEY_PRINTSCREEN),
					std::make_pair(utility::string_view("key-right"), Button::KEY_RIGHT),
					std::make_pair(utility::string_view("key-rightalt"), Button::KEY_RIGHTALT),
					std::make_pair(utility::string_view("key-rightbrace"), Button::KEY_RIGHTBRACE),
					std::make_pair(utility::string_view("key-rightctrl"), Button::KEY_RIGHTCTRL),
					std::make_pair(utility::string_view("key-rightmeta"), Button::KEY_RIGHTMETA),
					std::make_pair(utility::string_view("key-rightshift"), Button::KEY_RIGHTSHIFT),
					std::make_pair(utility::string_view("key-scrolllock"), Button::KEY_SCROLLLOCK),
					std::make_pair(utility::string_view("key-semicolon"), Button::KEY_SEMICOLON),
					std::make_pair(utility::string_view("key-slash"), Button::KEY_SLASH),
					std::make_pair(utility::string_view("key-space"), Button::KEY_SPACE),
					std::make_pair(utility::string_view("key-sysrq"), Button::KEY_SYSRQ),
					std::make_pair(utility::string_view("key-tab"), Button::KEY_TAB),
					std::make_pair(utility::string_view("key-up"), Button::KEY_UP)
				);
			}

			struct Position
			{
				int32_t x, y;
			};

		private:
			struct CommonHeader
			{
				State state;
				Player player;
			} common_header;
			static_assert(std::is_trivial<CommonHeader>::value, "");

			struct AxisValue
			{
				State state;
				Player player;
				Axis code;
				int8_t data;
				union
				{
					int32_t ivalue;
					uint32_t uvalue;
				};
			}  axis_value;
			static_assert(std::is_trivial<AxisValue>::value, "");

			struct ButtonState
			{
				State state;
				Player player;
				Button code;
				// int8_t count;
				// int24_t unused;
			} button_state;
			static_assert(std::is_trivial<ButtonState>::value, "");

			struct CursorMove
			{
				State state;
				Player player;
#if defined(_MSC_VER) && _MSC_VER <= 1916
				uint8_t xs[3];
				uint8_t ys[3];

				int32_t x() const { return (int32_t(xs[2]) << 24 | int32_t(xs[1]) << 16 | int32_t(xs[0]) << 8) >> 8; }
				int32_t y() const { return (int32_t(ys[2]) << 24 | int32_t(ys[1]) << 16 | int32_t(ys[0]) << 8) >> 8; }

				void x(int32_t value) { xs[0] = value & 0x000000ff; xs[1] = value >> 8 & 0x000000ff; xs[2] = value >> 16 & 0x000000ff; }
				void y(int32_t value) { ys[0] = value & 0x000000ff; ys[1] = value >> 8 & 0x000000ff; ys[2] = value >> 16 & 0x000000ff; }
#else
				int64_t x : 24;
				int64_t y : 24;
#endif
			} cursor_move;
			static_assert(std::is_trivial<CursorMove>::value, "");

			struct KeyCharacter
			{
				State state;
				Player player;
				Button code;
				utility::unicode_code_point unicode;
			} key_character;
			static_assert(std::is_trivial<KeyCharacter>::value, "");

		public:
			friend Input AxisTiltInput(int_fast8_t player, Axis code, int32_t value);
			friend Input AxisTriggerInput(int_fast8_t player, Axis code, uint32_t value);

			friend Input ButtonStateInput(int_fast8_t player, Button code, bool down);

			friend Input CursorMoveInput(int_fast8_t player, int_fast32_t x, int_fast32_t y);

			friend Input KeyCharacterInput(int_fast8_t player, Button code, utility::unicode_code_point unicode);

		public:
			/**
			 * \note Valid iff state is `AXIS_TILT`, or `AXIS_TRIGGER`.
			 */
			Axis getAxis() const
			{
				switch (common_header.state)
				{
				case State::AXIS_TILT: return axis_value.code;
				case State::AXIS_TRIGGER: return axis_value.code;
				default: debug_unreachable("invalid state");
				}
			}

			/**
			 * \note Valid iff state is `BUTTON_DOWN`, `BUTTON_UP`, or `KEY_CHARACTER`.
			 */
			Button getButton() const
			{
				switch (common_header.state)
				{
				case State::BUTTON_DOWN: return button_state.code;
				case State::BUTTON_UP: return button_state.code;
				case State::KEY_CHARACTER: return key_character.code;
				default: debug_unreachable("invalid state");
				}
			}

			int getDevice() const { return common_header.player; }

			/**
			 * \note Valid iff state is `CURSOR_MOVE`.
			 */
			Position getPosition() const
			{
				switch (common_header.state)
				{
#if defined(_MSC_VER) && _MSC_VER <= 1916
				case State::CURSOR_MOVE: return {cursor_move.x(), cursor_move.y()};
#else
				case State::CURSOR_MOVE: return {cursor_move.x, cursor_move.y};
#endif
				default: debug_unreachable("invalid state");
				}
			}

			State getState() const { return common_header.state; }

			/**
			 * \note Valid iff state is `AXIS_TILT`.
			 */
			int32_t getTilt() const
			{
				switch (common_header.state)
				{
				case State::AXIS_TILT: return axis_value.ivalue;
				default: debug_unreachable("invalid state");
				}
			}

			/**
			 * \note Valid iff state is `AXIS_TRIGGER`.
			 */
			uint32_t getTrigger() const
			{
				switch (common_header.state)
				{
				case State::AXIS_TRIGGER: return axis_value.uvalue;
				default: debug_unreachable("invalid state");
				}
			}

			/**
			 * \note Valid iff state is `KEY_CHARACTER`.
			 */
			utility::unicode_code_point getUnicode() const
			{
				switch (common_header.state)
				{
				case State::KEY_CHARACTER: return key_character.unicode;
				default: debug_unreachable("invalid state");
				}
			}

			/**
			 * \note Valid iff state is `AXIS_TILT`.
			 */
			int32_t getValueSigned() const
			{
				switch (common_header.state)
				{
				case State::AXIS_TILT: return axis_value.ivalue;
				default: debug_unreachable("invalid state");
				}
			}

			/**
			 * \note Valid iff state is `AXIS_TRIGGER`.
			 */
			uint32_t getValueUnsigned() const
			{
				switch (common_header.state)
				{
				case State::AXIS_TRIGGER: return axis_value.uvalue;
				default: debug_unreachable("invalid state");
				}
			}
		};
		static_assert(sizeof(Input) == 8, "This is not a hard requirement but it would be nice to know if it grows.");

		inline Input AxisTiltInput(int_fast8_t player, Input::Axis code, int32_t value)
		{
			Input input;
			input.axis_value.state = Input::State::AXIS_TILT;
			input.axis_value.player = player;
			input.axis_value.code = code;
			input.axis_value.ivalue = value;
			return input;
		}
		inline Input AxisTriggerInput(int_fast8_t player, Input::Axis code, uint32_t value)
		{
			Input input;
			input.axis_value.state = Input::State::AXIS_TRIGGER;
			input.axis_value.player = player;
			input.axis_value.code = code;
			input.axis_value.uvalue = value;
			return input;
		}

		inline Input ButtonStateInput(int_fast8_t player, Input::Button code, bool down)
		{
			Input input;
			input.button_state.state = down ? Input::State::BUTTON_DOWN : Input::State::BUTTON_UP;
			input.button_state.player = player;
			input.button_state.code = code;
			return input;
		}

		inline Input KeyCharacterInput(int_fast8_t player, Input::Button code, utility::unicode_code_point unicode)
		{
			Input input;
			input.key_character.state = Input::State::KEY_CHARACTER;
			input.key_character.player = player;
			input.key_character.code = code;
			input.key_character.unicode = unicode;
			return input;
		}

		inline Input CursorMoveInput(int_fast8_t player, int_fast32_t x, int_fast32_t y)
		{
			Input input;
			input.cursor_move.state = Input::State::CURSOR_MOVE;
			input.cursor_move.player = player;
#if defined(_MSC_VER) && _MSC_VER <= 1916
			input.cursor_move.x(x);
			input.cursor_move.y(y);
#else
			input.cursor_move.x = x;
			input.cursor_move.y = y;
#endif
			return input;
		}
	}
}

#endif /* ENGINE_HID_INPUT_HPP */
