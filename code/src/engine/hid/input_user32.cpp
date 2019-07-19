
#include <config.h>

#if WINDOW_USE_USER32

#include "input.hpp"

#include "utility/string.hpp"

#if INPUT_USE_RAWINPUT
# include <vector>
#endif

#include <Windows.h>

// #define PRINT_KEYBOARD_INPUT
// #define PRINT_MOUSE_INPUT
// #define PRINT_HID_INPUT

#if defined(PRINT_KEYBOARD_INPUT) || defined(PRINT_MOUSE_INPUT) || defined(PRINT_HID_INPUT)
# define PRINT_ANY_INPUT
#endif

namespace engine
{
#if INPUT_USE_RAWINPUT
	namespace application
	{
		namespace window
		{
			void RegisterRawInputDevices(const uint32_t * collections, int count);
		}
	}
#endif

	namespace hid
	{
		extern void dispatch(const Input & input);
	}
}

namespace
{
	engine::hid::Input input;

	using Button = engine::hid::Input::Button;

	// scancodes are the primary way we use to identify keys but we only
	// understand so many of the codes, and sometimes we purposefully do not
	// identify a certain key, so secondarily we look at the virtual key code
	// as well (see table below)
	// https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/scancode.doc
	// https://download.microsoft.com/download/1/6/1/161ba512-40e2-4cc9-843a-923143f3456c/translate.pdf
	// http://www.emc.com.tw/twn/database/Data_Sheet/PC/EM83053D.pdf
	// http://www.farnell.com/datasheets/79189.pdf
	// I guess these documents from 2k are about as new as it gets?
	const Button sc_to_button[256] =
	{
		Button::INVALID       , Button::KEY_ESC     , Button::KEY_1        , Button::KEY_2         , Button::KEY_3        , Button::KEY_4        , Button::KEY_5         , Button::KEY_6         ,
		Button::KEY_7         , Button::KEY_8       , Button::KEY_9        , Button::KEY_0         , Button::KEY_MINUS    , Button::KEY_EQUAL    , Button::KEY_BACKSPACE , Button::KEY_TAB       ,

		Button::KEY_Q         , Button::KEY_W       , Button::KEY_E        , Button::KEY_R         , Button::KEY_T        , Button::KEY_Y        , Button::KEY_U         , Button::KEY_I         ,
		Button::KEY_O         , Button::KEY_P       , Button::KEY_LEFTBRACE, Button::KEY_RIGHTBRACE, Button::KEY_ENTER    , Button::KEY_LEFTCTRL , Button::KEY_A         , Button::KEY_S         ,

		Button::KEY_D         , Button::KEY_F       , Button::KEY_G        , Button::KEY_H         , Button::KEY_J        , Button::KEY_K        , Button::KEY_L         , Button::KEY_SEMICOLON ,
		Button::KEY_APOSTROPHE, Button::KEY_GRAVE   , Button::KEY_LEFTSHIFT, Button::KEY_BACKSLASH , Button::KEY_Z        , Button::KEY_X        , Button::KEY_C         , Button::KEY_V         ,

		Button::KEY_B         , Button::KEY_N       , Button::KEY_M        , Button::KEY_COMMA     , Button::KEY_DOT      , Button::KEY_SLASH    , Button::KEY_RIGHTSHIFT, Button::KEY_KPASTERISK,
		Button::KEY_LEFTALT   , Button::KEY_SPACE   , Button::KEY_CAPSLOCK , Button::KEY_F1        , Button::KEY_F2       , Button::KEY_F3       , Button::KEY_F4        , Button::KEY_F5        ,

		Button::KEY_F6        , Button::KEY_F7      , Button::KEY_F8       , Button::KEY_F9        , Button::KEY_F10      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::KEY_KPMINUS  , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::KEY_KPPLUS    , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::KEY_SYSRQ    , Button::INVALID      , Button::KEY_102ND     , Button::KEY_F11       ,
		Button::KEY_F12       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::KEY_PAUSE    , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::KEY_F13      , Button::KEY_F14      , Button::KEY_F15       , Button::KEY_F16       ,
		Button::KEY_F17       , Button::KEY_F18     , Button::KEY_F19      , Button::KEY_F20       , Button::KEY_F21      , Button::KEY_F22      , Button::KEY_F23       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::KEY_F24       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::KEY_KPENTER  , Button::KEY_RIGHTCTRL, Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::KEY_KPSLASH  , Button::INVALID       , Button::INVALID       ,
		Button::KEY_RIGHTALT  , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::KEY_BREAK     , Button::KEY_HOME      ,
		Button::KEY_UP        , Button::KEY_PAGEUP  , Button::INVALID      , Button::KEY_LEFT      , Button::INVALID      , Button::KEY_RIGHT    , Button::INVALID       , Button::KEY_END       ,

		Button::KEY_DOWN      , Button::KEY_PAGEDOWN, Button::KEY_INSERT   , Button::KEY_DELETE    , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::KEY_LEFTMETA  , Button::KEY_RIGHTMETA, Button::KEY_COMPOSE  , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,

		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
		Button::INVALID       , Button::INVALID     , Button::INVALID      , Button::INVALID       , Button::INVALID      , Button::INVALID      , Button::INVALID       , Button::INVALID       ,
	};

	// note: not all entries are being used in this table, only when a button
	// is not found in the scancode table above will we ever look here, but we
	// fill it out anyway (to our best ability) because why not :shrug:
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	const Button vk_to_button[256] =
	{
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::KEY_BREAK   , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::KEY_BACKSPACE, Button::KEY_TAB       , Button::INVALID       , Button::INVALID     , Button::KEY_CLEAR      , Button::KEY_ENTER  , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::KEY_PAUSE   , Button::KEY_CAPSLOCK   , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::KEY_ESC     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::KEY_SPACE    , Button::KEY_PAGEUP    , Button::KEY_PAGEDOWN  , Button::KEY_END     , Button::KEY_HOME       , Button::KEY_LEFT   , Button::KEY_UP    , Button::KEY_RIGHT  ,
		Button::KEY_DOWN     , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::KEY_PRINTSCREEN, Button::KEY_INSERT , Button::KEY_DELETE, Button::INVALID    ,

		Button::KEY_0        , Button::KEY_1         , Button::KEY_2         , Button::KEY_3       , Button::KEY_4          , Button::KEY_5      , Button::KEY_6     , Button::KEY_7      ,
		Button::KEY_8        , Button::KEY_9         , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::KEY_A         , Button::KEY_B         , Button::KEY_C       , Button::KEY_D          , Button::KEY_E      , Button::KEY_F     , Button::KEY_G      ,
		Button::KEY_H        , Button::KEY_I         , Button::KEY_J         , Button::KEY_K       , Button::KEY_L          , Button::KEY_M      , Button::KEY_N     , Button::KEY_O      ,

		Button::KEY_P        , Button::KEY_Q         , Button::KEY_R         , Button::KEY_S       , Button::KEY_T          , Button::KEY_U      , Button::KEY_V     , Button::KEY_W      ,
		Button::KEY_X        , Button::KEY_Y         , Button::KEY_Z         , Button::KEY_LEFTMETA, Button::KEY_RIGHTMETA  , Button::KEY_COMPOSE, Button::INVALID   , Button::INVALID    ,

		Button::KEY_KP0      , Button::KEY_KP1       , Button::KEY_KP2       , Button::KEY_KP3     , Button::KEY_KP4        , Button::KEY_KP5    , Button::KEY_KP6   , Button::KEY_KP7    ,
		Button::KEY_KP8      , Button::KEY_KP9       , Button::KEY_KPASTERISK, Button::KEY_KPPLUS  , Button::INVALID        , Button::KEY_MINUS  , Button::KEY_KPDOT , Button::KEY_KPSLASH,

		Button::KEY_F1       , Button::KEY_F2        , Button::KEY_F3        , Button::KEY_F4      , Button::KEY_F5         , Button::KEY_F6     , Button::KEY_F7    , Button::KEY_F8     ,
		Button::KEY_F9       , Button::KEY_F10       , Button::KEY_F11       , Button::KEY_F12     , Button::KEY_F13        , Button::KEY_F14    , Button::KEY_F15   , Button::KEY_F16    ,

		Button::KEY_F17      , Button::KEY_F18       , Button::KEY_F19       , Button::KEY_F20     , Button::KEY_F21        , Button::KEY_F22    , Button::KEY_F23   , Button::KEY_F24    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::KEY_NUMLOCK  , Button::KEY_SCROLLLOCK, Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,

		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
		Button::INVALID      , Button::INVALID       , Button::INVALID       , Button::INVALID     , Button::INVALID        , Button::INVALID    , Button::INVALID   , Button::INVALID    ,
	};

#if INPUT_USE_RAWINPUT
	std::vector<HANDLE> devices;
#endif
}

namespace engine
{
	namespace hid
	{
#if INPUT_USE_RAWINPUT
		void add_device(HANDLE device)
		{
			debug_assert(std::find(devices.begin(), devices.end(), device) == devices.end(), "device has already been added!");

			char name[256] = "Unknown";
			{
				UINT len = sizeof name;
				const auto ret = GetRawInputDeviceInfo(device, RIDI_DEVICENAME, name, &len);
				debug_assert(ret >= 0, "buffer too small, expected ", len);
			}

			RID_DEVICE_INFO rdi;
			rdi.cbSize = sizeof rdi;
			UINT len = sizeof rdi;
			debug_verify(GetRawInputDeviceInfo(device, RIDI_DEVICEINFO, &rdi, &len) == sizeof rdi);

			devices.push_back(device);

			switch (rdi.dwType)
			{
			case RIM_TYPEMOUSE:
				debug_printline("device ", devices.size() - 1, "(mouse) added: \"", name, "\" id ", rdi.mouse.dwId, " buttons ", rdi.mouse.dwNumberOfButtons, " sample rate ", rdi.mouse.dwSampleRate, rdi.mouse.fHasHorizontalWheel ? " has horizontal wheel" : "");
				break;
			case RIM_TYPEKEYBOARD:
				debug_printline("device ", devices.size() - 1, "(keyboard) added: \"", name, "\" type ", rdi.keyboard.dwType, " sub type ", rdi.keyboard.dwSubType, " mode ", rdi.keyboard.dwKeyboardMode, " function keys ", rdi.keyboard.dwNumberOfFunctionKeys, " indicators ", rdi.keyboard.dwNumberOfIndicators, " keys ", rdi.keyboard.dwNumberOfKeysTotal);
				break;
			case RIM_TYPEHID:
				debug_printline("device ", devices.size() - 1, "(hid) added: \"", name, "\" vendor id ", rdi.hid.dwVendorId, " product id ", rdi.hid.dwProductId, " version ", rdi.hid.dwVersionNumber, " usage page ", rdi.hid.usUsagePage, " usage ", rdi.hid.usUsage);
				break;
			default: debug_unreachable();
			}
		}

		void remove_device(HANDLE device)
		{
			auto it = std::find(devices.begin(), devices.end(), device);
			debug_assert(it != devices.end(), "device was never added before removal!");
			devices.erase(it);
		}

		void create()
		{
			// collection numbers
			// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections-opened-by-windows-for-system-use
			// awsome document that explains the numbers
			// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
			uint32_t collections[] = {
				0x00010002, // mouse
				0x00010006, // keyboard
				0x000c0001, // consumer audio control - for things like volume buttons
			};
			engine::application::window::RegisterRawInputDevices(collections, sizeof collections / sizeof collections[0]);
		}

		void destroy()
		{}

		void process_input(HRAWINPUT input)
		{
			UINT len = 0;
			debug_verify(GetRawInputData(input, RID_INPUT, nullptr, &len, sizeof(RAWINPUTHEADER)) == 0);

			std::vector<char> bytes; // what about alignment?
			bytes.resize(len);
			debug_verify(GetRawInputData(input, RID_INPUT, bytes.data(), &len, sizeof(RAWINPUTHEADER)) == len);

			const RAWINPUT & ri = *reinterpret_cast<const RAWINPUT *>(bytes.data());
			auto it = std::find(devices.begin(), devices.end(), ri.header.hDevice);
			// debug_assert(it != devices.end(), "received input from unknown device!");

# ifdef PRINT_ANY_INPUT
			std::string info = "device ";
			info += it == devices.end() ? "x" : utility::to_string(it - devices.begin());

			switch (ri.header.dwType)
			{
#  ifdef PRINT_MOUSE_INFO
			case RIM_TYPEMOUSE:
				info += utility::to_string("(mouse): ", ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE ? " absolute {" : " relative {", ri.data.mouse.lLastX, ", ", ri.data.mouse.lLastY, "}");
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN)
				{
					info += " left down";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_UP)
				{
					info += " left up";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_DOWN)
				{
					info += " right down";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_2_UP)
				{
					info += " right up";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_DOWN)
				{
					info += " middle down";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_3_UP)
				{
					info += " middle up";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN)
				{
					info += " side down";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP)
				{
					info += " side up";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN)
				{
					info += " extra down";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP)
				{
					info += " extra up";
				}
				if (ri.data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
				{
					info += utility::to_string(" wheel delta ", static_cast<SHORT>(ri.data.mouse.usButtonData));
				}
				info += utility::to_string(" buttons ", ri.data.mouse.ulRawButtons, " extra ", ri.data.mouse.ulExtraInformation);
				break;
#  endif
#  ifdef PRINT_KEYBOARD_INFO
			case RIM_TYPEKEYBOARD:
				info += utility::to_string(" (keyboard): scan code ", ri.data.keyboard.MakeCode, ri.data.keyboard.Flags & RI_KEY_BREAK ? " up" : " down");
				if (ri.data.keyboard.Flags & RI_KEY_E0)
				{
					info += " e0";
				}
				if (ri.data.keyboard.Flags & RI_KEY_E1)
				{
					info += " e1";
				}
				info += utility::to_string(" virtual key ", ri.data.keyboard.VKey, " message ", ri.data.keyboard.Message, " extra ", ri.data.mouse.ulExtraInformation);
				break;
#  endif
#  ifdef PRINT_HID_INFO
			case RIM_TYPEHID:
				info += utility::to_string(" (hid): contains ", ri.data.hid.dwCount, " packages");
				for (int i = 0; i < ri.data.hid.dwCount; i++)
				{
					info += utility::to_string(" ", i, "{");
					for (int j = 0; j < ri.data.hid.dwSizeHid; j++)
					{
						if (j > 0)
						{
							info += ' ';
						}

						const char hexs[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };
						const uint8_t byte = ri.data.hid.bRawData[i * ri.data.hid.dwSizeHid + j];
						info += hexs[(byte >> 4) & 0x0f];
						info += hexs[byte & 0x0f];
					}
					info += '}';
				}
				break;
#  endif
			default:
				info.clear();
			}
			if (!info.empty())
			{
				debug_printline(info);
			}
# endif

			if (it == devices.end())
				return;

			switch (ri.header.dwType)
			{
			case RIM_TYPEMOUSE:
				break;
			case RIM_TYPEKEYBOARD:
			{
				static_assert(RI_KEY_E0 == 2, "");
				static_assert(RI_KEY_E1 == 4, "");
				// set the most significant bit if e0 is set, and the second
				// most significant bit if e1 is set
				const int sc = ri.data.keyboard.MakeCode | ((ri.data.keyboard.Flags & RI_KEY_E0) << 6) | ((ri.data.keyboard.Flags & RI_KEY_E1) << 4);
				const Button sc_button = sc_to_button[sc];
				// note: pause is the only button with the e1 prefix (as far as
				// I can tell) so in order for it to not collide with ctrl (as
				// it has the same scancode as ctrl :facepalm:) we set the
				// second most significant bit in that case, thus transforming
				// `1d` to `5d` which is unused! for completness, right ctrl
				// `e0 1d` transforms to `9d`, and left ctrl remains as `1d`

				const int vk = ri.data.keyboard.VKey;
				const Button vk_button = vk_to_button[vk];

				const Button button = sc_button != Button::INVALID ? sc_button : vk_button;

				const auto button_name = core::value_table<Button>::get_key(button);
				debug_printline(button_name);
				break;
			}
			case RIM_TYPEHID:
				break;
			}
		}
#endif

		void key_character(int scancode, const char16_t * u16)
		{
			input.setKeyCharacter(0, sc_to_button[scancode], utility::code_point(u16));
			dispatch(input);
		}

#if !INPUT_USE_RAWINPUT
		void key_down(WPARAM wParam, LPARAM lParam, LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
		}
		void key_up(WPARAM wParam, LPARAM lParam, LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
		}
		void syskey_down(WPARAM wParam, LPARAM lParam, LONG time)
		{
			input.setButtonDown(0, engine::hid::Input::Button::KEY_0, 0, 0);
			dispatch(input);
		}
		void syskey_up(WPARAM wParam, LPARAM lParam, LONG time)
		{
			input.setButtonUp(0, engine::hid::Input::Button::KEY_0, 0, 0);
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
#endif
	}
}

#endif /* WINDOW_USE_USER32 */
