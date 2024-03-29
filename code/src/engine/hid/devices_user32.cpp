#include "config.h"

#if WINDOW_USE_USER32

#include "engine/console.hpp"
#include "engine/hid/devices.hpp"
#include "engine/hid/input.hpp"

#include "utility/ext/stddef.hpp"
#include "utility/ranges.hpp"

#include "ful/cstr.hpp"
#include "ful/heap.hpp"
#include "ful/static.hpp"
#include "ful/string_modify.hpp"
#include "ful/convert.hpp"

#if INPUT_HAS_USER32_RAWINPUT
# include "utility/container/array.hpp"
# include "utility/container/vector.hpp"
#endif

#include <Windows.h>
#if INPUT_HAS_USER32_RAWINPUT && INPUT_HAS_USER32_HID
# include <Hidsdi.h>
#endif

// history (more interesting than anything else)
// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/introduction-to-hid-concepts
// reference page (kind of broken but good to have)
// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/_hid/

// #define PRINT_HID_INFO
// #define PRINT_KEYBOARD_INFO
// #define PRINT_MOUSE_INFO
// #define PRINT_HID_INFO

#if defined(PRINT_KEYBOARD_INFO) || defined(PRINT_MOUSE_INFO) || defined(PRINT_HID_INFO)
# define PRINT_ANY_INFO
#endif

namespace engine
{
#if INPUT_HAS_USER32_RAWINPUT
	namespace application
	{
		extern void RegisterRawInputDevices(window & window, const uint32_t * collections, int count);
		extern void UnregisterRawInputDevices(window & window, const uint32_t * collections, int count);
	}
#endif

	namespace hid
	{
		extern void found_device(int id, int vendor, int product);
		extern void lost_device(int id);

		extern void add_source(int id, ful::heap_string_utf8 && path, int type, ful::view_utf8 name);
		extern void remove_source(int id, ful::heap_string_utf8 && path);

		extern void dispatch(const Input & input);
	}
}

namespace
{
	engine::application::window * window = nullptr;

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

#if INPUT_HAS_USER32_RAWINPUT
	struct Device
	{
		HANDLE handle;

		engine::hid::Input::Player id;

		Device(HANDLE handle)
			: handle(handle)
		{
			static engine::hid::Input::Player next_id = 1; // 0 is reserved for not hardware input
			debug_assert(next_id <= engine::hid::Input::max_player, "too many devices has been added, maybe we need to recycle some ids?");
			id = next_id++;
		}
	};

	utility::heap_vector<Device> devices;

# if INPUT_HAS_USER32_HID
	struct Format
	{
		struct Field
		{
			int nbits;
			int usage;

			int type;
			int index;
		};

		utility::heap_array<Format::Field> fields;
		utility::heap_array<HIDP_BUTTON_CAPS> button_caps;
		utility::heap_array<HIDP_VALUE_CAPS> value_caps;
	};
	utility::heap_vector<Format> formats;
# endif
#endif

	const engine::hid::Input::Button ds4_buttons[] = {
		engine::hid::Input::Button::INVALID,
		engine::hid::Input::Button::GAMEPAD_WEST, // SQUARE
		engine::hid::Input::Button::GAMEPAD_SOUTH, // CROSS
		engine::hid::Input::Button::GAMEPAD_EAST, // CIRCLE
		engine::hid::Input::Button::GAMEPAD_NORTH, // TRIANGLE
		engine::hid::Input::Button::GAMEPAD_TL, // L1
		engine::hid::Input::Button::GAMEPAD_TR, // R1
		engine::hid::Input::Button::GAMEPAD_TL2, // L2
		engine::hid::Input::Button::GAMEPAD_TR2, // R2
		engine::hid::Input::Button::GAMEPAD_SELECT, // SHARE
		engine::hid::Input::Button::GAMEPAD_START, // OPTIONS
		engine::hid::Input::Button::GAMEPAD_THUMBL, // L3
		engine::hid::Input::Button::GAMEPAD_THUMBR, // R3
		engine::hid::Input::Button::GAMEPAD_MODE, // PS
		engine::hid::Input::Button::MOUSE_LEFT, // TOUCH
	};

	const ful::cstr_utf8 ds4_values[] = {
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"), ful::cstr_utf8("unused"),
		ful::cstr_utf8("LS-x"),
		ful::cstr_utf8("LS-y"),
		ful::cstr_utf8("RS-x"),
		ful::cstr_utf8("L2-z"),
		ful::cstr_utf8("R2-z"),
		ful::cstr_utf8("RS-y"),
		ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"),
		ful::cstr_utf8("unused"),
		ful::cstr_utf8("D-PAD"),
	};

	engine::hid::Input::Button get_button(int scan_code, int virtual_key)
	{
		const Button sc_button = sc_to_button[scan_code];
		const Button vk_button = vk_to_button[virtual_key];

		return sc_button != Button::INVALID ? sc_button : vk_button;
	}

#if INPUT_HAS_USER32_RAWINPUT
	std::atomic_int hardware_input(0);

	// collection numbers
	// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections-opened-by-windows-for-system-use
	// awsome document that explains the numbers
	// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
	const uint32_t collections[] = {
		0x00010002, // mouse
		0x00010006, // keyboard
		0x000c0001, // consumer audio control - for things like volume buttons
# if INPUT_HAS_USER32_HID
		0x00010004, // joystick
		0x00010005, // gamepad
# endif
	};

	void start_hardware_input()
	{
		RegisterRawInputDevices(*::window, collections, sizeof collections / sizeof collections[0]);
	}

	void stop_hardware_input()
	{
		UnregisterRawInputDevices(*::window, collections, sizeof collections / sizeof collections[0]);

		// todo which thread are we executing this in? it should be
		// the same one as for add/remove_device
		for (auto i : utility::reverse(ranges::index_sequence_for(devices)))
		{
			engine::hid::lost_device(devices[i].id);
		}
		devices.clear();
	}

	int lock_state_variable(std::atomic_int & state)
	{
		int value;
		do
		{
			value = state.load(std::memory_order_relaxed);
			while (!state.compare_exchange_weak(value, -1, std::memory_order_relaxed));
		} while (value == -1);

		return value;
	}

	void disable_hardware_input()
	{
		if (lock_state_variable(hardware_input) != 0)
		{
			stop_hardware_input();
		}
		hardware_input.store(0, std::memory_order_relaxed);
	}

	void enable_hardware_input()
	{
		if (lock_state_variable(hardware_input) != 1)
		{
			start_hardware_input();
		}
		hardware_input.store(1, std::memory_order_relaxed);
	}

	void disable_hardware_input_callback(void * /*data*/)
	{
		disable_hardware_input();
	}

	void enable_hardware_input_callback(void * /*data*/)
	{
		enable_hardware_input();
	}

	void toggle_hardware_input_callback(void * /*data*/)
	{
		int value = lock_state_variable(hardware_input);
		switch (value)
		{
		case 0:
			value = 1;
			start_hardware_input();
			break;
		case 1:
			value = 0;
			stop_hardware_input();
			break;
		default:
			debug_unreachable();
		}
		hardware_input.store(value, std::memory_order_relaxed);
	}
#endif
}

namespace engine
{
	namespace hid
	{
#if INPUT_HAS_USER32_RAWINPUT
		void add_device(devices & /*devices*/, HANDLE handle)
		{
			debug_assert(std::find_if(::devices.begin(), ::devices.end(), [&handle](const Device & device){ return device.handle == handle; }) == ::devices.end(), "device has already been added!");

			ful::static_string_utf8<256> name;
			{
				wchar_t namew[256] = L"Unknown (buffer too small)";
				UINT len = sizeof namew;
				const auto ret = GetRawInputDeviceInfoW(handle, RIDI_DEVICENAME, namew, &len);
				debug_assert(ret >= UINT(0), "buffer too small, expected ", len);
				debug_verify(ful::convert(namew, namew + ret / sizeof(wchar_t), name));
			}

			RID_DEVICE_INFO rdi;
			{
				rdi.cbSize = sizeof rdi;
				UINT len = sizeof rdi;
				debug_verify(GetRawInputDeviceInfoW(handle, RIDI_DEVICEINFO, &rdi, &len) == sizeof rdi);
			}

			::devices.try_emplace_back(handle);
			Device & device = ext::back(::devices);
# if INPUT_HAS_USER32_HID
			formats.try_emplace_back();
# endif

			switch (rdi.dwType)
			{
			case RIM_TYPEMOUSE:
				debug_printline("device ", ::devices.size() - 1, "(mouse) added: \"", name, "\" id ", rdi.mouse.dwId, " buttons ", rdi.mouse.dwNumberOfButtons, " sample rate ", rdi.mouse.dwSampleRate, rdi.mouse.fHasHorizontalWheel ? ful::cstr_utf8(" has horizontal wheel") : ful::cstr_utf8(""));
				found_device(device.id, 0, 0);
				add_source(device.id, ful::heap_string_utf8(), 3, ful::view_utf8(name));
				break;
			case RIM_TYPEKEYBOARD:
				debug_printline("device ", ::devices.size() - 1, "(keyboard) added: \"", name, "\" type ", rdi.keyboard.dwType, " sub type ", rdi.keyboard.dwSubType, " mode ", rdi.keyboard.dwKeyboardMode, " function keys ", rdi.keyboard.dwNumberOfFunctionKeys, " indicators ", rdi.keyboard.dwNumberOfIndicators, " keys ", rdi.keyboard.dwNumberOfKeysTotal);
				found_device(device.id, 0, 0);
				add_source(device.id, ful::heap_string_utf8(), 2, ful::view_utf8(name));
				break;
# if INPUT_HAS_USER32_HID
			case RIM_TYPEHID:
			{
				debug_printline("device ", ::devices.size() - 1, "(hid) added: \"", name, "\" vendor id ", rdi.hid.dwVendorId, " product id ", rdi.hid.dwProductId, " version ", rdi.hid.dwVersionNumber, " usage page ", rdi.hid.usUsagePage, " usage ", rdi.hid.usUsage);
				found_device(device.id, rdi.hid.dwVendorId, rdi.hid.dwProductId);
				add_source(device.id, ful::heap_string_utf8(), 0, ful::view_utf8(name));

				PHIDP_PREPARSED_DATA preparsed_data = nullptr;
				UINT len;
				debug_verify(GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, nullptr, &len) == 0);
				std::vector<BYTE> bytes(len); // todo alignment
				debug_verify(GetRawInputDeviceInfoW(handle, RIDI_PREPARSEDDATA, bytes.data(), &len) == bytes.size());
				preparsed_data = reinterpret_cast<PHIDP_PREPARSED_DATA>(bytes.data());

				HIDP_CAPS caps;
				HidP_GetCaps(preparsed_data, &caps);

				auto & fields = ext::back(formats).fields;
				fields.resize(caps.NumberInputDataIndices + caps.NumberOutputDataIndices + caps.NumberFeatureDataIndices);
				const int field_offsets[] = { 0, caps.NumberInputDataIndices, caps.NumberInputDataIndices + caps.NumberOutputDataIndices };

				auto print_button_cap = [](const HIDP_BUTTON_CAPS & button_cap) {
					debug_printline("  usage page ", button_cap.UsagePage);
					debug_printline("  report id ", int(button_cap.ReportID));
					debug_printline("  bit field ", button_cap.BitField);
					debug_printline("  is absolute ", button_cap.IsAbsolute ? ful::cstr_utf8("yes") : ful::cstr_utf8("no"));
					debug_printline("  is range ", button_cap.IsRange ? ful::cstr_utf8("yes") : ful::cstr_utf8("no"));
					if (button_cap.IsRange)
					{
						debug_printline("  usage min ", button_cap.Range.UsageMin, " max ", button_cap.Range.UsageMax);
						debug_printline("  data index min ", button_cap.Range.DataIndexMin, " max ", button_cap.Range.DataIndexMax);
					}
					else
					{
						debug_printline("  usage ", button_cap.NotRange.Usage);
						debug_printline("  data index ", button_cap.NotRange.DataIndex);
					}
					if (button_cap.IsStringRange)
					{
						debug_printline("  string index min ", button_cap.Range.StringMin, " max ", button_cap.Range.StringMax);
					}
					else
					{
						debug_printline("  string index ", button_cap.NotRange.StringIndex);
					}
					if (button_cap.IsDesignatorRange)
					{
						debug_printline("  designator index min ", button_cap.Range.DesignatorMin, " max ", button_cap.Range.DesignatorMax);
					}
					else
					{
						debug_printline("  designator index ", button_cap.NotRange.DesignatorIndex);
					}
				};

				auto & button_caps = ext::back(formats).button_caps;
				button_caps.resize(caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps + caps.NumberFeatureButtonCaps);
				USHORT nbutton_caps;
				HidP_GetButtonCaps(HidP_Input, button_caps.data(), &nbutton_caps, preparsed_data);
				HidP_GetButtonCaps(HidP_Output, button_caps.data() + caps.NumberInputButtonCaps, &nbutton_caps, preparsed_data);
				HidP_GetButtonCaps(HidP_Feature, button_caps.data() + caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps, &nbutton_caps, preparsed_data);

				for (ext::usize i : ranges::index_sequence_for(button_caps))
				{
					const auto & button_cap = button_caps[i];
					const int offsets[] = { 0, caps.NumberInputButtonCaps, caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps };
					const HIDP_REPORT_TYPE type = i < static_cast<ext::usize>(offsets[1]) ? HidP_Input : i < static_cast<ext::usize>(offsets[2]) ? HidP_Output : HidP_Feature;

# if PRINT_HID_INFO
					debug_printline("button ", i, ":");
					print_button_cap(button_cap);
# endif

					if (button_cap.IsRange)
					{
						for (int r = 0; r < button_cap.Range.DataIndexMax - button_cap.Range.DataIndexMin + 1; r++)
						{
							const int data_index = button_cap.Range.DataIndexMin + r;
							fields[field_offsets[type] + data_index].nbits = 1;
							fields[field_offsets[type] + data_index].usage = button_cap.Range.UsageMin + r;
							fields[field_offsets[type] + data_index].type = 0;
							fields[field_offsets[type] + data_index].index = static_cast<int>(i);
						}
					}
					else
					{
						const int data_index = button_cap.NotRange.DataIndex;
						fields[field_offsets[type] + data_index].nbits = 1;
						fields[field_offsets[type] + data_index].usage = button_cap.NotRange.Usage;
						fields[field_offsets[type] + data_index].type = 0;
						fields[field_offsets[type] + data_index].index = static_cast<int>(i);
					}
				}

				auto print_value_cap = [](const HIDP_VALUE_CAPS & value_cap) {
					debug_printline("  usage page ", value_cap.UsagePage);
					debug_printline("  report id ", int(value_cap.ReportID));
					debug_printline("  bit field ", value_cap.BitField);
					debug_printline("  is absolute ", value_cap.IsAbsolute ? ful::cstr_utf8("yes") : ful::cstr_utf8("no"));
					debug_printline("  is range ", value_cap.IsRange ? ful::cstr_utf8("yes") : ful::cstr_utf8("no"));
					debug_printline("  bit size ", value_cap.BitSize);
					debug_printline("  report count ", value_cap.ReportCount);
					debug_printline("  units exp ", value_cap.UnitsExp);
					debug_printline("  units ", value_cap.Units);
					debug_printline("  logical min ", value_cap.LogicalMin);
					debug_printline("  logical max ", value_cap.LogicalMax);
					debug_printline("  physical min ", value_cap.PhysicalMin);
					debug_printline("  physical max ", value_cap.PhysicalMax);
					if (value_cap.IsRange)
					{
						debug_printline("  usage min ", value_cap.Range.UsageMin, " max ", value_cap.Range.UsageMax);
						debug_printline("  data index min ", value_cap.Range.DataIndexMin, " max ", value_cap.Range.DataIndexMax);
					}
					else
					{
						debug_printline("  usage ", value_cap.NotRange.Usage);
						debug_printline("  data index ", value_cap.NotRange.DataIndex);
					}
					if (value_cap.IsStringRange)
					{
						debug_printline("  string index min ", value_cap.Range.StringMin, " max ", value_cap.Range.StringMax);
					}
					else
					{
						debug_printline("  string index ", value_cap.NotRange.StringIndex);
					}
					if (value_cap.IsDesignatorRange)
					{
						debug_printline("  designator index min ", value_cap.Range.DesignatorMin, " max ", value_cap.Range.DesignatorMax);
					}
					else
					{
						debug_printline("  designator index ", value_cap.NotRange.DesignatorIndex);
					}
				};

				auto & value_caps = ext::back(formats).value_caps;
				value_caps.resize(caps.NumberInputValueCaps + caps.NumberOutputValueCaps + caps.NumberFeatureValueCaps);
				USHORT nvalue_caps;
				HidP_GetValueCaps(HidP_Input, value_caps.data(), &nvalue_caps, preparsed_data);
				HidP_GetValueCaps(HidP_Output, value_caps.data() + caps.NumberInputValueCaps, &nvalue_caps, preparsed_data);
				HidP_GetValueCaps(HidP_Feature, value_caps.data() + caps.NumberInputValueCaps + caps.NumberOutputValueCaps, &nvalue_caps, preparsed_data);

				for (ext::usize i : ranges::index_sequence_for(value_caps))
				{
					const auto & value_cap = value_caps[i];
					const int offsets[] = { 0, caps.NumberInputValueCaps, caps.NumberInputValueCaps + caps.NumberOutputValueCaps };
					const HIDP_REPORT_TYPE type = i < static_cast<ext::usize>(offsets[1]) ? HidP_Input : i < static_cast<ext::usize>(offsets[2]) ? HidP_Output : HidP_Feature;

# if PRINT_HID_INFO
					debug_printline("value ", i, ":");
					print_value_cap(value_cap);
# endif

					if (value_cap.IsRange)
					{
						for (int r = 0; r < value_cap.Range.DataIndexMax - value_cap.Range.DataIndexMin + 1; r++)
						{
							const int data_index = value_cap.Range.DataIndexMin + r;
							fields[field_offsets[type] + data_index].nbits = value_cap.BitSize;
							fields[field_offsets[type] + data_index].usage = value_cap.Range.UsageMin + r;
							fields[field_offsets[type] + data_index].type = 1;
							fields[field_offsets[type] + data_index].index = static_cast<int>(i);
						}
					}
					else
					{
						const int data_index = value_cap.NotRange.DataIndex;
						fields[field_offsets[type] + data_index].nbits = value_cap.BitSize;
						fields[field_offsets[type] + data_index].usage = value_cap.NotRange.Usage;
						fields[field_offsets[type] + data_index].type = 1;
						fields[field_offsets[type] + data_index].index = static_cast<int>(i);
					}
				}

				// temporary throw everything away that is not an input
				fields.resize(field_offsets[1]);

				int bit_offset = 0;
				for (ext::usize i : ranges::index_sequence_for(fields))
				{
					const auto & field = fields[i];
					const HIDP_REPORT_TYPE type = i < static_cast<ext::usize>(field_offsets[1]) ? HidP_Input : i < static_cast<ext::usize>(field_offsets[2]) ? HidP_Output : HidP_Feature;
					const ful::cstr_utf8 type_names[] = {ful::cstr_utf8("input"), ful::cstr_utf8("output"), ful::cstr_utf8("feature")};

					if (field.type)
					{
						const auto & value_cap = value_caps[field.index];

						int flatness = 0;
						if (value_cap.LinkUsagePage == 0x0001 && (value_cap.LinkUsage == 0x0004 || value_cap.LinkUsage == 0x0005))
						{
							// set flatness to something arbitrary like `(b - a) >> 4` I guess :shrug:
							// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-input.c#L1181
							flatness = (value_cap.LogicalMax - value_cap.LogicalMin) >> 4;
						}

						if (value_cap.UsagePage == 0x0001)
						{
							if (field.usage < sizeof ds4_values / sizeof ds4_values[0])
							{
								debug_printline("  ", ds4_values[field.usage]);
							}
							else
							{
								debug_printline("  ", "weird");
							}
						}
					}
					else
					{
						const auto & button_cap = button_caps[field.index];
						if (button_cap.UsagePage == 0x0009)
						{
							const auto button = ds4_buttons[field.usage];
							const auto button_name = core::value_table<Button>::get_key(button);
							debug_printline("  ", button_name);
						}
					}

					bit_offset += field.nbits;
				}
				break;
			}
# endif
			default: debug_unreachable();
			}
		}

		void remove_device(devices & /*devices*/, HANDLE handle)
		{
			auto it = std::find_if(::devices.begin(), ::devices.end(), [&handle](const Device & device){ return device.handle == handle; });
			debug_assert(it != ::devices.end(), "device was never added before removal!");

			lost_device(it->id);

# if INPUT_HAS_USER32_HID
			formats.erase(std::next(formats.begin(), std::distance(::devices.begin(), it)));
# endif
			::devices.erase(it);
		}

		void destroy_subsystem(devices & /*devices*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			disable_hardware_input();
#endif

			engine::abandon(ful::cstr_utf8("toggle-hardware-input"));
			engine::abandon(ful::cstr_utf8("enable-hardware-input"));
			engine::abandon(ful::cstr_utf8("disable-hardware-input"));

			lost_device(0); // non hardware device

			::window = nullptr;
		}

		void create_subsystem(devices & /*devices*/, engine::application::window & window_, bool hardware_input_)
		{
			::window = &window_;

			found_device(0, 0, 0); // non hardware device

#if INPUT_HAS_USER32_RAWINPUT
			engine::observe(ful::cstr_utf8("disable-hardware-input"), disable_hardware_input_callback, nullptr);
			engine::observe(ful::cstr_utf8("enable-hardware-input"), enable_hardware_input_callback, nullptr);
			engine::observe(ful::cstr_utf8("toggle-hardware-input"), toggle_hardware_input_callback, nullptr);

			if (hardware_input_)
			{
				enable_hardware_input();
			}
#endif
		}

		void process_input(devices & /*devices*/, HRAWINPUT input)
		{
			UINT len = 0;
			debug_verify(GetRawInputData(input, RID_INPUT, nullptr, &len, sizeof(RAWINPUTHEADER)) == 0);

			utility::heap_array<char> bytes; // what about alignment?
			debug_verify(bytes.resize(len));
			debug_verify(GetRawInputData(input, RID_INPUT, bytes.data(), &len, sizeof(RAWINPUTHEADER)) == len);

			const RAWINPUT & ri = *reinterpret_cast<const RAWINPUT *>(bytes.data());
			// ri.header.hDevice may be null, according to stack overflow user
			// 175201, yet the documentation mentions nothing of it. When it is
			// null, there does not seem to be possible to get the id of the
			// device that sent the data, so all we can do is ignore it :shrug:
			// great design microsoft
			//
			// https://stackoverflow.com/a/57616263
			auto it = std::find_if(::devices.begin(), ::devices.end(), [&ri](const Device & device){ return device.handle == ri.header.hDevice; });
			// debug_assert(it != devices.end(), "received input from unknown device!");

# ifdef PRINT_ANY_INFO
			std::string info = "device ";
			info += it == ::devices.end() ? "x" : utility::to_string(it - ::devices.begin());

			switch (ri.header.dwType)
			{
#  ifdef PRINT_MOUSE_INFO
			case RIM_TYPEMOUSE:
				info += utility::to_string(" (mouse): ", ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE ? " absolute {" : " relative {", ri.data.mouse.lLastX, ", ", ri.data.mouse.lLastY, "}");
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

			if (it == ::devices.end())
				return;

			switch (ri.header.dwType)
			{
			case RIM_TYPEMOUSE:
			{
				const engine::hid::Input::Button buttons[] =
				{
					engine::hid::Input::Button::MOUSE_LEFT,
					engine::hid::Input::Button::MOUSE_RIGHT,
					engine::hid::Input::Button::MOUSE_MIDDLE,
					engine::hid::Input::Button::MOUSE_SIDE,
					engine::hid::Input::Button::MOUSE_EXTRA,
				};

				for (int i = 0; i < sizeof buttons / sizeof buttons[0]; i++)
				{
					if (ri.data.mouse.usButtonFlags & 1 << i * 2)
					{
						dispatch(ButtonStateInput(it->id, buttons[i], true));
					}
					if (ri.data.mouse.usButtonFlags & 2 << i * 2)
					{
						dispatch(ButtonStateInput(it->id, buttons[i], false));
					}
				}
				break;
			}
			case RIM_TYPEKEYBOARD:
			{
				static_assert(RI_KEY_E0 == 2, "The bit shift for e0 is off!");
				static_assert(RI_KEY_E1 == 4, "The bit shift for e1 is off!");
				// set the most significant bit if e0 is set, and the second
				// most significant bit if e1 is set
				const int sc = ri.data.keyboard.MakeCode | ((ri.data.keyboard.Flags & RI_KEY_E0) << 6) | ((ri.data.keyboard.Flags & RI_KEY_E1) << 4);
				// note: pause is the only button with the e1 prefix (as far as
				// I can tell) so in order for it to not collide with ctrl (as
				// it has the same scancode as ctrl :facepalm:) we set the
				// second most significant bit in that case, thus transforming
				// `1d` to `5d` which is unused! for completness, right ctrl
				// `e0 1d` transforms to `9d`, and left ctrl remains as `1d`

				dispatch(ButtonStateInput(it->id, get_button(sc, ri.data.keyboard.VKey), ri.data.keyboard.Flags & RI_KEY_BREAK ? false : true));
				break;
			}
# if INPUT_HAS_USER32_HID
			case RIM_TYPEHID:
			{
				// ds4 specific stuff
				if (ri.data.hid.dwSizeHid == 64 && ri.data.hid.bRawData[0] == 1)
				{
					// todo: check vendor id and device id
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-ids.h#L1025

					// based on dualshock4_parse_report
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-sony.c#L919

					const int time_offset = 10;
					const uint16_t timestamp = (uint16_t(ri.data.hid.bRawData[time_offset + 1]) << 8) | uint16_t(ri.data.hid.bRawData[time_offset]);

					// todo: gyro and accelerometer
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-sony.c#L1005
					// in order to make this useful, we have to get the calibration
					// data from the device but how? the linux kernel writes a
					// request directly to the device (see link)
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-sony.c#L1583
					// and then parses the data, on windows things appear to not be
					// that simple, this function looks promising
					// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/hidsdi/nf-hidsdi-hidd_getfeature
					// but we cannot call it (I think) because we have a device
					// handle, not a handle to a device object :/ maybe this
					// specific feature data we want is already in the parsed data?

					// you get pointed to hclient for examples on the windows docs
					// alot when reading on their hid pages, but it is not very
					// easy to understand what is going on
					// https://github.com/microsoft/Windows-driver-samples/blob/master/hid/hclient/hclient.c

					const int battery_offset = 30;
					const bool cable_attached = ((uint8_t(ri.data.hid.bRawData[battery_offset]) >> 4) & 0x01) != 0;
					int battery_capacity = ri.data.hid.bRawData[battery_offset] & 0x0f;

					const bool battery_charging = cable_attached && battery_capacity <= 10;

					if (!cable_attached)
					{
						battery_capacity++;
					}
					battery_capacity = std::min(battery_capacity, 10);
					battery_capacity *= 10;

					const int touchpad_offset = 33;

					const uint8_t time_of_touch = ri.data.hid.bRawData[touchpad_offset + 1];
					const bool touch1 = (ri.data.hid.bRawData[touchpad_offset + 2] & 0x80) == 0;
					if (touch1)
					{
						const uint16_t x1 = ((uint16_t(ri.data.hid.bRawData[touchpad_offset + 4]) & 0x0f) << 8) | uint16_t(ri.data.hid.bRawData[touchpad_offset + 3]);
						const uint16_t y1 = (uint16_t(ri.data.hid.bRawData[touchpad_offset + 5]) << 4) | (uint16_t(ri.data.hid.bRawData[touchpad_offset + 4]) >> 4);
					}
					const bool touch2 = (ri.data.hid.bRawData[touchpad_offset + 6] & 0x80) == 0;
					if (touch2)
					{
						const uint16_t x2 = ((uint16_t(ri.data.hid.bRawData[touchpad_offset + 8]) & 0x0f) << 8) | uint16_t(ri.data.hid.bRawData[touchpad_offset + 7]);
						const uint16_t y2 = (uint16_t(ri.data.hid.bRawData[touchpad_offset + 9]) << 4) | (uint16_t(ri.data.hid.bRawData[touchpad_offset + 8]) >> 4);
					}
				}

				int bit_offset = CHAR_BIT; // skip the first byte
				for (const auto & field : formats[it - ::devices.begin()].fields)
				{
					const int from = bit_offset / CHAR_BIT;
					const int to = (bit_offset + field.nbits + (CHAR_BIT - 1)) / CHAR_BIT;
					debug_assert(std::size_t(to - from) <= sizeof(uint64_t));

					uint64_t value = 0;
					for (int i = 0; i < to - from; i++)
					{
						value |= uint64_t(ri.data.hid.bRawData[from + i]) << (i * CHAR_BIT);
					}
					value = value >> (bit_offset % CHAR_BIT);
					value = value & ((uint64_t(1) << field.nbits) - 1);

					bit_offset += field.nbits;
				}
				break;
			}
# endif
			}
		}
#endif

		void key_character(devices & /*devices*/, int scancode, ful::point_utf character)
		{
			const engine::hid::Input::Button button = sc_to_button[scancode];
			dispatch(KeyCharacterInput(0, button, character));
		}

		void key_down(devices & /*devices*/, WPARAM wParam, LPARAM lParam, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, get_button((uint32_t(lParam & 0x00ff0000) >> 16) | (uint32_t(lParam & 0x01000000) >> 17), static_cast<int>(wParam)), true));
		}

		void key_up(devices & /*devices*/, WPARAM wParam, LPARAM lParam, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, get_button((uint32_t(lParam & 0x00ff0000) >> 16) | (uint32_t(lParam & 0x01000000) >> 17), static_cast<int>(wParam)), false));
		}

		void syskey_down(devices & /*devices*/, WPARAM wParam, LPARAM lParam, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, get_button((uint32_t(lParam & 0x00ff0000) >> 16) | (uint32_t(lParam & 0x01000000) >> 17), static_cast<int>(wParam)), true));
		}

		void syskey_up(devices & /*devices*/, WPARAM wParam, LPARAM lParam, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, get_button((uint32_t(lParam & 0x00ff0000) >> 16) | (uint32_t(lParam & 0x01000000) >> 17), static_cast<int>(wParam)), false));
		}

		void lbutton_down(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_LEFT, true));
		}

		void lbutton_up(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_LEFT, false));
		}

		void mbutton_down(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_MIDDLE, true));
		}

		void mbutton_up(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_MIDDLE, false));
		}

		void rbutton_down(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_RIGHT, true));
		}

		void rbutton_up(devices & /*devices*/, LONG /*time*/)
		{
#if INPUT_HAS_USER32_RAWINPUT
			if (hardware_input.load(std::memory_order_relaxed))
				return;
#endif
			dispatch(ButtonStateInput(0, engine::hid::Input::Button::MOUSE_RIGHT, false));
		}

		void mouse_move(devices & /*devices*/, int_fast16_t x, int_fast16_t y, LONG /*time*/)
		{
			dispatch(CursorMoveInput(0, x, y));
		}

		void mouse_wheel(devices & /*devices*/, int_fast16_t /*delta*/, LONG /*time*/)
		{
			// TODO:
		}
	}
}

#endif /* WINDOW_USE_USER32 */
