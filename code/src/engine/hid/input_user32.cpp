
#include <config.h>

#if WINDOW_USE_USER32

#include "input.hpp"

#include "utility/string.hpp"

#if INPUT_USE_RAWINPUT
# include <vector>
#endif

#include <Windows.h>
#if INPUT_USE_HID
# include <Hidsdi.h>
#endif

// history (more interesting than anything else)
// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/introduction-to-hid-concepts
// reference page (kind of broken but good to have)
// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/_hid/

// #define PRINT_HID_INFO
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

# if INPUT_USE_HID
	struct Format
	{
		struct Field
		{
			int nbits;
			int usage;

			int type;
			int index;
		};

		std::vector<Format::Field> fields;
		std::vector<HIDP_BUTTON_CAPS> button_caps;
		std::vector<HIDP_VALUE_CAPS> value_caps;
	};
	std::vector<Format> formats;
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

	const char * const ds4_values[] = {
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"unused", "unused", "unused", "unused", "unused", "unused", "unused", "unused",
		"LS-x",
		"LS-y",
		"RS-x",
		"L2-z",
		"R2-z",
		"RS-y",
		"unused",
		"unused",
		"unused",
		"D-PAD",
	};
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
# if INPUT_USE_HID
			formats.emplace_back();
# endif

			switch (rdi.dwType)
			{
			case RIM_TYPEMOUSE:
				debug_printline("device ", devices.size() - 1, "(mouse) added: \"", name, "\" id ", rdi.mouse.dwId, " buttons ", rdi.mouse.dwNumberOfButtons, " sample rate ", rdi.mouse.dwSampleRate, rdi.mouse.fHasHorizontalWheel ? " has horizontal wheel" : "");
				break;
			case RIM_TYPEKEYBOARD:
				debug_printline("device ", devices.size() - 1, "(keyboard) added: \"", name, "\" type ", rdi.keyboard.dwType, " sub type ", rdi.keyboard.dwSubType, " mode ", rdi.keyboard.dwKeyboardMode, " function keys ", rdi.keyboard.dwNumberOfFunctionKeys, " indicators ", rdi.keyboard.dwNumberOfIndicators, " keys ", rdi.keyboard.dwNumberOfKeysTotal);
				break;
# if INPUT_USE_HID
			case RIM_TYPEHID:
			{
				debug_printline("device ", devices.size() - 1, "(hid) added: \"", name, "\" vendor id ", rdi.hid.dwVendorId, " product id ", rdi.hid.dwProductId, " version ", rdi.hid.dwVersionNumber, " usage page ", rdi.hid.usUsagePage, " usage ", rdi.hid.usUsage);

				PHIDP_PREPARSED_DATA preparsed_data = nullptr;
				UINT len;
				debug_verify(GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, nullptr, &len) == 0);
				std::vector<BYTE> bytes(len); // alignment?
				debug_verify(GetRawInputDeviceInfo(device, RIDI_PREPARSEDDATA, bytes.data(), &len) == bytes.size());
				preparsed_data = reinterpret_cast<PHIDP_PREPARSED_DATA>(bytes.data());

				HIDP_CAPS caps;
				HidP_GetCaps(preparsed_data, &caps);

				auto & fields = formats.back().fields;
				fields.resize(caps.NumberInputDataIndices + caps.NumberOutputDataIndices + caps.NumberFeatureDataIndices);
				const int field_offsets[] = { 0, caps.NumberInputDataIndices, caps.NumberInputDataIndices + caps.NumberOutputDataIndices };

				auto print_button_cap = [](const HIDP_BUTTON_CAPS & button_cap) {
					debug_printline("  usage page ", button_cap.UsagePage);
					debug_printline("  report id ", int(button_cap.ReportID));
					debug_printline("  bit field ", button_cap.BitField);
					debug_printline("  is absolute ", button_cap.IsAbsolute ? "yes" : "no");
					debug_printline("  is range ", button_cap.IsRange ? "yes" : "no");
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

				auto & button_caps = formats.back().button_caps;
				button_caps.resize(caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps + caps.NumberFeatureButtonCaps);
				USHORT nbutton_caps;
				HidP_GetButtonCaps(HidP_Input, button_caps.data(), &nbutton_caps, preparsed_data);
				HidP_GetButtonCaps(HidP_Output, button_caps.data() + caps.NumberInputButtonCaps, &nbutton_caps, preparsed_data);
				HidP_GetButtonCaps(HidP_Feature, button_caps.data() + caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps, &nbutton_caps, preparsed_data);

				for (int i = 0; i < button_caps.size(); i++)
				{
					const auto & button_cap = button_caps[i];
					const int offsets[] = { 0, caps.NumberInputButtonCaps, caps.NumberInputButtonCaps + caps.NumberOutputButtonCaps };
					const HIDP_REPORT_TYPE type = i < offsets[1] ? HidP_Input : i < offsets[2] ? HidP_Output : HidP_Feature;

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
							fields[field_offsets[type] + data_index].index = i;
						}
					}
					else
					{
						const int data_index = button_cap.NotRange.DataIndex;
						fields[field_offsets[type] + data_index].nbits = 1;
						fields[field_offsets[type] + data_index].usage = button_cap.NotRange.Usage;
						fields[field_offsets[type] + data_index].type = 0;
						fields[field_offsets[type] + data_index].index = i;
					}
				}

				auto print_value_cap = [](const HIDP_VALUE_CAPS & value_cap) {
					debug_printline("  usage page ", value_cap.UsagePage);
					debug_printline("  report id ", int(value_cap.ReportID));
					debug_printline("  bit field ", value_cap.BitField);
					debug_printline("  is absolute ", value_cap.IsAbsolute ? "yes" : "no");
					debug_printline("  is range ", value_cap.IsRange ? "yes" : "no");
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

				auto & value_caps = formats.back().value_caps;
				value_caps.resize(caps.NumberInputValueCaps + caps.NumberOutputValueCaps + caps.NumberFeatureValueCaps);
				USHORT nvalue_caps;
				HidP_GetValueCaps(HidP_Input, value_caps.data(), &nvalue_caps, preparsed_data);
				HidP_GetValueCaps(HidP_Output, value_caps.data() + caps.NumberInputValueCaps, &nvalue_caps, preparsed_data);
				HidP_GetValueCaps(HidP_Feature, value_caps.data() + caps.NumberInputValueCaps + caps.NumberOutputValueCaps, &nvalue_caps, preparsed_data);

				for (int i = 0; i < value_caps.size(); i++)
				{
					const auto & value_cap = value_caps[i];
					const int offsets[] = { 0, caps.NumberInputValueCaps, caps.NumberInputValueCaps + caps.NumberOutputValueCaps };
					const HIDP_REPORT_TYPE type = i < offsets[1] ? HidP_Input : i < offsets[2] ? HidP_Output : HidP_Feature;

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
							fields[field_offsets[type] + data_index].index = i;
						}
					}
					else
					{
						const int data_index = value_cap.NotRange.DataIndex;
						fields[field_offsets[type] + data_index].nbits = value_cap.BitSize;
						fields[field_offsets[type] + data_index].usage = value_cap.NotRange.Usage;
						fields[field_offsets[type] + data_index].type = 1;
						fields[field_offsets[type] + data_index].index = i;
					}
				}

				// temporary throw everything away that is not an input
				fields.resize(field_offsets[1]);

				int bit_offset = 0;
				for (int i = 0; i < fields.size(); i++)
				{
					const auto & field = fields[i];
					const HIDP_REPORT_TYPE type = i < field_offsets[1] ? HidP_Input : i < field_offsets[2] ? HidP_Output : HidP_Feature;
					const char * const type_names[] = { "input", "output", "feature" };
					const char * const name = field.type ? "value" : "button";
					debug_printline("field ", i, "(", type_names[type], " ", name, ") ", field.nbits, " bits, starting at bit ", bit_offset);

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

		void remove_device(HANDLE device)
		{
			auto it = std::find(devices.begin(), devices.end(), device);
			debug_assert(it != devices.end(), "device was never added before removal!");
# if INPUT_USE_HID
			formats.erase(std::next(formats.begin(), std::distance(devices.begin(), it)));
# endif
			devices.erase(it);
		}

		void create(bool hardware_input)
		{
			// collection numbers
			// https://docs.microsoft.com/en-us/windows-hardware/drivers/hid/top-level-collections-opened-by-windows-for-system-use
			// awsome document that explains the numbers
			// https://www.usb.org/sites/default/files/documents/hut1_12v2.pdf
			uint32_t collections[] = {
				0x00010002, // mouse
				0x00010006, // keyboard
				0x000c0001, // consumer audio control - for things like volume buttons
# if INPUT_USE_HID
				0x00010004, // joystick
				0x00010005, // gamepad
# endif
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
# if INPUT_USE_HID
			case RIM_TYPEHID:
			{
				// ds4 specific stuff
				if (ri.data.hid.dwSizeHid == 64 && ri.data.hid.bRawData[0] == 1)
				{
					// todo: check vendor id and device id
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-ids.h#L1025

					// based on dualshock4_parse_report
					// https://github.com/torvalds/linux/blob/9637d517347e80ee2fe1c5d8ce45ba1b88d8b5cd/drivers/hid/hid-sony.c#L919

					std::string data = utility::to_string("device ", it - devices.begin(), " ds4 specifics:");

					const int time_offset = 10;
					const uint16_t timestamp = (uint16_t(ri.data.hid.bRawData[time_offset + 1]) << 8) | uint16_t(ri.data.hid.bRawData[time_offset]);
					data += utility::to_string(" timestamp ", timestamp);

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
					data += utility::to_string(" battery ", battery_capacity, "%");
					if (battery_charging)
					{
						data += "(charging)";
					}

					const int touchpad_offset = 33;
					int ntouches = ri.data.hid.bRawData[touchpad_offset];
					data += utility::to_string(" touches ", ntouches);

					const uint8_t time_of_touch = ri.data.hid.bRawData[touchpad_offset + 1];
					const bool touch1 = (ri.data.hid.bRawData[touchpad_offset + 2] & 0x80) == 0;
					if (touch1)
					{
						const uint16_t x1 = ((uint16_t(ri.data.hid.bRawData[touchpad_offset + 4]) & 0x0f) << 8) | uint16_t(ri.data.hid.bRawData[touchpad_offset + 3]);
						const uint16_t y1 = (uint16_t(ri.data.hid.bRawData[touchpad_offset + 5]) << 4) | (uint16_t(ri.data.hid.bRawData[touchpad_offset + 4]) >> 4);
						data += utility::to_string(" 1{", x1, ", ", y1, "}");
					}
					const bool touch2 = (ri.data.hid.bRawData[touchpad_offset + 6] & 0x80) == 0;
					if (touch2)
					{
						const uint16_t x2 = ((uint16_t(ri.data.hid.bRawData[touchpad_offset + 8]) & 0x0f) << 8) | uint16_t(ri.data.hid.bRawData[touchpad_offset + 7]);
						const uint16_t y2 = (uint16_t(ri.data.hid.bRawData[touchpad_offset + 9]) << 4) | (uint16_t(ri.data.hid.bRawData[touchpad_offset + 8]) >> 4);
						data += utility::to_string(" 2{", x2, ", ", y2, "}");
					}

					debug_printline(data);
				}

				std::string data = utility::to_string("device ", it - devices.begin(), " fields:");

				int bit_offset = CHAR_BIT; // skip the first byte
				for (const auto & field : formats[it - devices.begin()].fields)
				{
					const int from = bit_offset / CHAR_BIT;
					const int to = (bit_offset + field.nbits + (CHAR_BIT - 1)) / CHAR_BIT;
					debug_assert(to - from <= sizeof(uint64_t));

					uint64_t value = 0;
					for (int i = 0; i < to - from; i++)
					{
						value |= uint64_t(ri.data.hid.bRawData[from + i]) << (i * CHAR_BIT);
					}
					value = value >> (bit_offset % CHAR_BIT);
					value = value & ((uint64_t(1) << field.nbits) - 1);

					data += ' ';
					for (int i = 0; i < field.nbits; i++)
					{
						data += value & (uint64_t(1) << i) ? '1' : '0';
					}

					bit_offset += field.nbits;
				}
				debug_printline(data);
				break;
			}
# endif
			}
		}
#endif

		void key_character(int scancode, const char16_t * u16)
		{
			const engine::hid::Input::Button button = sc_to_button[scancode];
			dispatch(KeyCharacterInput(0, button, utility::code_point(u16)));
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
