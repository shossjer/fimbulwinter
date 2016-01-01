
#ifndef ENGINE_HID_INPUT_HPP
#define ENGINE_HID_INPUT_HPP

#include <config.h>

#include <cstdint>

namespace engine
{
	namespace hid
	{
		/**
		 */
		class Input
		{
		public:
			/**
			 */
			class Additional
			{
			public:
				/**
				 */
				enum Flag
				{
					ALT               = 0x0001,
					CTRL              = 0x0002,
					SHIFT             = 0x0004,
					WIN               = 0x0008,
					CAPSLOCK          = 0x0010,
					LEFTMOUSEBUTTON   = 0x0020,
					MIDDLEMOUSEBUTTON = 0x0040,
					RIGHTMOUSEBUTTON  = 0x0080,
					XTRAMOUSEBUTTON1  = 0x0100,
					XTRAMOUSEBUTTON2  = 0x0200
				};

			private:
				/**
				 */
				uint16_t mask;

			public:
				/**
				 */
				Additional() : mask(0) {}
				/**
				 */
				Additional(const Additional & additional) = default;
				/**
				 */
				Additional(const uint_fast16_t mask) : mask(mask) {}

				/**
				 */
				Additional & operator = (const Additional & additional) = default;

			public:
				/**
				 */
				bool operator == (const Additional & additional) const
				{
					return mask == additional.mask;
				}
				/**
				 */
				bool operator != (const Additional & additional) const
				{
					return mask != additional.mask;
				}

			public:
				/**
				 */
				void reset(Flag flag)
				{
					mask &= ~(uint16_t)flag;
				}
				/**
				 */
				void set(Flag flag)
				{
					mask |= (uint16_t)flag;
				}
				/**
				 */
				void toggle(Flag flag)
				{
					mask ^= (uint16_t)flag;
				}
			};
			/**
			 */
			enum class Button
			{
#if WINDOW_USE_USER32
				NIL             = 0x00,
				KEY_0           = 0x30,
				KEY_1           = 0x31,
				KEY_2           = 0x32,
				KEY_3           = 0x33,
				KEY_4           = 0x34,
				KEY_5           = 0x35,
				KEY_6           = 0x36,
				KEY_7           = 0x37,
				KEY_8           = 0x38,
				KEY_9           = 0x39,
				KEY_A           = 0x41,
				KEY_B           = 0x42,
				KEY_C           = 0x43,
				KEY_D           = 0x44,
				KEY_E           = 0x45,
				KEY_F           = 0x46,
				KEY_G           = 0x47,
				KEY_H           = 0x48,
				KEY_I           = 0x49,
				KEY_J           = 0x4A,
				KEY_K           = 0x4B,
				KEY_L           = 0x4C,
				KEY_M           = 0x4D,
				KEY_N           = 0x4E,
				KEY_O           = 0x4F,
				KEY_P           = 0x50,
				KEY_Q           = 0x51,
				KEY_R           = 0x52,
				KEY_S           = 0x53,
				KEY_T           = 0x54,
				KEY_U           = 0x55,
				KEY_V           = 0x56,
				KEY_W           = 0x57,
				KEY_X           = 0x58,
				KEY_Y           = 0x59,
				KEY_Z           = 0x5A,
				KEY_ADD         = 0x6B,
				KEY_ALT_LEFT    = 0x12,
				KEY_ALT_RIGHT   = 0x12,
				KEY_ARROWDOWN   = 0x28,
				KEY_ARROWLEFT   = 0x25,
				KEY_ARROWRIGHT  = 0x27,
				KEY_ARROWUP     = 0x26,
				KEY_BACKSPACE   = 0x08,
				KEY_CAPSLOCK    = 0x14,
				KEY_CTRL_LEFT   = 0x11,
				KEY_CTRL_RIGHT  = 0x11,
				KEY_DECIMAL     = 0x6E,
				KEY_DEL         = 0x2E,
				KEY_DIVIDE      = 0x6F,
				KEY_END         = 0x23,
				KEY_ESCAPE      = 0x1B,
				KEY_F1          = 0x70,
				KEY_F2          = 0x71,
				KEY_F3          = 0x72,
				KEY_F4          = 0x73,
				KEY_F5          = 0x74,
				KEY_F6          = 0x75,
				KEY_F7          = 0x76,
				KEY_F8          = 0x77,
				KEY_F9          = 0x78,
				KEY_F10         = 0x79,
				KEY_F11         = 0x7A,
				KEY_F12         = 0x7B,
				KEY_F13         = 0x7C,
				KEY_F14         = 0x7D,
				KEY_F15         = 0x7E,
				KEY_F16         = 0x7F,
				KEY_F17         = 0x80,
				KEY_F18         = 0x81,
				KEY_F19         = 0x82,
				KEY_F20         = 0x83,
				KEY_F21         = 0x84,
				KEY_F22         = 0x85,
				KEY_F23         = 0x86,
				KEY_F24         = 0x87,
				KEY_HOME        = 0x24,
				KEY_INS         = 0x2D,
				KEY_MISC1       = 0xBA,
				KEY_MISC2       = 0xBF,
				KEY_MISC3       = 0xC0,
				KEY_MISC4       = 0xDB,
				KEY_MISC5       = 0xDC,
				KEY_MISC6       = 0xDD,
				KEY_MISC7       = 0xDE,
				KEY_MISC8       = 0xDF,
				KEY_MULTIPLY    = 0x6A,
				KEY_NUMLOCK     = 0x90,
				KEY_NUMPAD0     = 0x60,
				KEY_NUMPAD1     = 0x61,
				KEY_NUMPAD2     = 0x62,
				KEY_NUMPAD3     = 0x63,
				KEY_NUMPAD4     = 0x64,
				KEY_NUMPAD5     = 0x65,
				KEY_NUMPAD6     = 0x66,
				KEY_NUMPAD7     = 0x67,
				KEY_NUMPAD8     = 0x68,
				KEY_NUMPAD9     = 0x69,
				KEY_PAGEDOWN    = 0x22,
				KEY_PAGEUP      = 0x21,
				KEY_PAUSE       = 0x13,
				KEY_PRINTSCREEN = 0x2C,
				KEY_RETURN      = 0x0D,
				KEY_SCROLLLOCK  = 0x91,
				KEY_SHIFT_LEFT  = 0x10,
				KEY_SHIFT_RIGHT = 0x10,
				KEY_SPACEBAR    = 0x20,
				KEY_SUBTRACT    = 0x6D,
				KEY_TAB         = 0x09,
				KEY_WIN_LEFT    = 0x5B,
				KEY_WIN_RIGHT   = 0x5C,
				MOUSE_LEFT      = 0x01,
				MOUSE_MIDDLE    = 0x04,
				MOUSE_RIGHT     = 0x02,
				MOUSE_X1        = 0x05,
				MOUSE_X2        = 0x06
#elif WINDOW_USE_X11
				NIL             = 0x00,
				KEY_0           = 0x13,
				KEY_1           = 0x0A,
				KEY_2           = 0x0B,
				KEY_3           = 0x0C,
				KEY_4           = 0x0D,
				KEY_5           = 0x0E,
				KEY_6           = 0x0F,
				KEY_7           = 0x10,
				KEY_8           = 0x11,
				KEY_9           = 0x12,
				KEY_A           = 0x26,
				KEY_B           = 0x38,
				KEY_C           = 0x36,
				KEY_D           = 0x28,
				KEY_E           = 0x1A,
				KEY_F           = 0x29,
				KEY_G           = 0x2A,
				KEY_H           = 0x2B,
				KEY_I           = 0x1F,
				KEY_J           = 0x2C,
				KEY_K           = 0x2D,
				KEY_L           = 0x2E,
				KEY_M           = 0x3A,
				KEY_N           = 0x39,
				KEY_O           = 0x20,
				KEY_P           = 0x21,
				KEY_Q           = 0x18,
				KEY_R           = 0x1B,
				KEY_S           = 0x27,
				KEY_T           = 0x1C,
				KEY_U           = 0x1E,
				KEY_V           = 0x37,
				KEY_W           = 0x19,
				KEY_X           = 0x35,
				KEY_Y           = 0x1D,
				KEY_Z           = 0x34,
				KEY_ADD         = 0x14,
				KEY_ALT_LEFT    = 0x40,
				KEY_ALT_RIGHT   = 0x6C, // MAYBE
				KEY_ARROWDOWN   = 0x74,
				KEY_ARROWLEFT   = 0x71,
				KEY_ARROWRIGHT  = 0x72,
				KEY_ARROWUP     = 0x6F,
				KEY_BACKSPACE   = 0x16,
				KEY_CAPSLOCK    = 0x42,
				KEY_CTRL_LEFT   = 0x37,
				KEY_CTRL_RIGHT  = 0x69,
				KEY_DECIMAL     = 0x81,
				KEY_DEL         = 0x77,
				KEY_DIVIDE      = 0x6A,
				KEY_END         = 0x73,
				KEY_ESCAPE      = 0x09,
				KEY_F1          = 0x43,
				KEY_F2          = 0x44,
				KEY_F3          = 0x45,
				KEY_F4          = 0x46,
				KEY_F5          = 0x47,
				KEY_F6          = 0x48,
				KEY_F7          = 0x49,
				KEY_F8          = 0x4A,
				KEY_F9          = 0x4B,
				KEY_F10         = 0x4C,
				KEY_F11         = 0x5F,
				KEY_F12         = 0x60,
				KEY_F13         = 0x00,
				KEY_F14         = 0x00,
				KEY_F15         = 0x00,
				KEY_F16         = 0x00,
				KEY_F17         = 0x00,
				KEY_F18         = 0x00,
				KEY_F19         = 0x00,
				KEY_F20         = 0x00,
				KEY_F21         = 0x00,
				KEY_F22         = 0x00,
				KEY_F23         = 0x00,
				KEY_F24         = 0x00,
				KEY_HOME        = 0x6E,
				KEY_INS         = 0x76,
				KEY_MISC1       = 0x00,
				KEY_MISC2       = 0x00,
				KEY_MISC3       = 0x00,
				KEY_MISC4       = 0x00,
				KEY_MISC5       = 0x00,
				KEY_MISC6       = 0x00,
				KEY_MISC7       = 0x00,
				KEY_MISC8       = 0x00,
				KEY_MULTIPLY    = 0x3F,
				KEY_NUMLOCK     = 0x4D,
				KEY_NUMPAD0     = 0x5A,
				KEY_NUMPAD1     = 0x57,
				KEY_NUMPAD2     = 0x58,
				KEY_NUMPAD3     = 0x59,
				KEY_NUMPAD4     = 0x53,
				KEY_NUMPAD5     = 0x54,
				KEY_NUMPAD6     = 0x55,
				KEY_NUMPAD7     = 0x4F,
				KEY_NUMPAD8     = 0x50,
				KEY_NUMPAD9     = 0x51,
				KEY_PAGEDOWN    = 0x75,
				KEY_PAGEUP      = 0x6F,
				KEY_PAUSE       = 0x7F,
				KEY_PRINTSCREEN = 0x6B,
				KEY_RETURN      = 0x24,
				KEY_SCROLLLOCK  = 0x4E,
				KEY_SHIFT_LEFT  = 0x32,
				KEY_SHIFT_RIGHT = 0x3E,
				KEY_SPACEBAR    = 0x41,
				KEY_SUBTRACT    = 0x52,
				KEY_TAB         = 0x17,
				KEY_WIN_LEFT    = 0x85,
				KEY_WIN_RIGHT   = 0x86,
				MOUSE_LEFT      = 0x01,
				MOUSE_MIDDLE    = 0x02,
				MOUSE_RIGHT     = 0x03,
				MOUSE_X1        = 0x00,
				MOUSE_X2        = 0x00
#endif
				/*
				  CANCEL    = 0x03, // control-break processing
				  CLEAR     = 0x0C, // CLEAR key
				  SELECT    = 0x29, // SELECT key
				  PRINT     = 0x2A, // PRINT key
				  EXECUTE   = 0x2B, // EXECUTE key
				  HELP      = 0x2F, // HELP key
				  SLEEP     = 0x5F, // computer sleep key
				  SEPARATOR = 0x6C, // separator key
				*/
			};
			/**
			 */
			struct Code
			{
#if WINDOW_USE_USER32
				/**
				 */
				BYTE virtual_key;
				/**
				 */
				BYTE scan_code;
#elif WINDOW_USE_X11
				/**
				 */
				uint16_t keycode;
				/**
				 */
				uint16_t state;
#endif
			};
			/**
			 */
			struct Cursor
			{
				/**
				 */
				int16_t x, y;
			};
			/**
			 */
			struct Move
			{
				/**
				 */
				int16_t dx, dy;
			};
			/**
			 */
			enum class State
			{
				DOWN,
				MOVE,
				UP,
				WHEEL
			};
			/**
			 */
			struct Wheel
			{
				// TODO: add appropriate data here
			};

		private:
			/**
			 */
			Cursor cursor;
			/**
			 */
			Additional additional;
			/**
			 */
			State state;
			/**
			 */
			union
			{
				/**
				 */
				Code code;
				/**
				 */
				Move move;
				/**
				 */
				Wheel wheel;
			} data;

		public:
			/**
			 */
			Input() = default;
			/**
			 */
			Input(const Input & input) = delete;

			/**
			 */
			Input & operator = (const Input & input) = delete;

		public:
			/**
			 * Gets the additional keys.
			 */
			Additional getAdditional() const { return this->additional; }
			/**
			 * \note This is only valid if the state of the message is `DOWN`.
			 */
			char getASCII() const
			{
#if WINDOW_USE_USER32
				WORD c = 0;
				{
					BYTE keystate[256];
					{
						GetKeyboardState((PBYTE)keystate);
					}
					ToAscii((UINT)this->data.code.virtual_key,
					        (UINT)this->data.code.scan_code,
					        keystate,
					        &c,
					        0);
					// ToAsciiEx((UINT)data.code.virtual_key, (UINT)data.code.scan_code, keystate, &c, 0, GetKeyboardLayout(0));
				}
				return (char)c;
#elif WINDOW_USE_X11
				return 0; // TODO: to do
#endif
			}
			/**
			 * Gets the button.
			 *
			 * \note This is only valid if the state of the message is either `DOWN` or `UP`.
			 */
			Button getButton() const
			{
#if WINDOW_USE_USER32
				return (Button)this->data.code.virtual_key;
#elif WINDOW_USE_X11
				return (Button)this->data.code.keycode;
#endif
			}
			/**
			 * Gets the mouse cursor.
			 */
			Cursor getCursor() const { return this->cursor; }
			/**
			 * Gets the movement of the mouse.
			 *
			 * \note This is only valid if the state of the message is `MOVE`.
			 */
			Move getMove() const { return this->data.move; }
			/**
			 * Gets the state.
			 */
			State getState() const { return this->state; }
			/**
			 * Gets the wheel movement of the mouse.
			 *
			 * \note This is only valid if the state of the message is `WHEEL`.
			 */
			Wheel getWheel() const { return this->data.wheel; }

			/**
			 */
			void setCursor(const int_fast16_t x, const int_fast16_t y)
			{
				this->cursor.x = x;
				this->cursor.y = y;
			}

#if WINDOW_USE_USER32
			/**
			 * Sets input to a `DOWN` state.
			 *
			 * \param[in] virtual_key Windows specific value.
			 * \param[in] scan_code   Windows specific value.
			 */
			void setDown(BYTE virtual_key, BYTE scan_code)
			{
				this->state = State::DOWN;

				this->data.code.virtual_key = virtual_key;
				this->data.code.scan_code = scan_code;
			}
			/**
			 * Sets input to a `UP` state.
			 *
			 * \param[in] virtual_key Windows specific value.
			 * \param[in] scan_code   Windows specific value.
			 */
			void setUp(BYTE virtual_key, BYTE scan_code)
			{
				this->state = State::UP;

				this->data.code.virtual_key = virtual_key;
				this->data.code.scan_code = scan_code;
			}
#elif WINDOW_USE_X11
			/**
			 * Sets input to a `DOWN` state.
			 *
			 * \param[in] keycode Linux specific value.
			 * \param[in] state   Linux specific value.
			 */
			void setDown(const unsigned int keycode, const unsigned int state)
			{
				this->state = State::DOWN;

				this->data.code.keycode = keycode;
				this->data.code.state = state;
			}
			/**
			 * Sets input to a `UP` state.
			 *
			 * \param[in] keycode Linux specific value.
			 * \param[in] state   Linux specific value.
			 */
			void setUp(const unsigned int keycode, const unsigned int state)
			{
				this->state = State::UP;

				this->data.code.keycode = keycode;
				this->data.code.state = state;
			}
#endif
			/**
			 * Sets input to a `MOVE` state.
			 *
			 * \param[in] dx The x-movement of the cursor.
			 * \param[in] dy The y-movement of the cursor.
			 */
			void setMove(const int_fast16_t dx, const int_fast16_t dy)
			{
				this->state = State::MOVE;

				this->data.move.dx = dx;
				this->data.move.dy = dy;
			}
			/**
			 * Sets input to a `WHEEL` state.
			 *
			 * \param[in] delta The movement of the wheel.
			 */
			void setWheel(const int delta)
			{
				this->state = State::WHEEL;

				// data.wheel.set(delta);
			}
		};

		/**
		 */
		void dispatch(const Input & input);
	}
}

#endif /* ENGINE_HID_INPUT_HPP */
