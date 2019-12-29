#include "config.h"

#if WINDOW_USE_X11

#include "devices.hpp"

#include "core/async/Thread.hpp"
#include "core/maths/util.hpp"

#include "engine/Asset.hpp"
#include "engine/console.hpp"
#include "engine/debug.hpp"
#include "engine/hid/input.hpp"

#include "utility/algorithm.hpp"
#include "utility/unicode.hpp"

#include <climits>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/inotify.h>
#include <unistd.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>

namespace engine
{
	namespace application
	{
		extern XkbDescPtr load_key_names(window & window);
		extern void free_key_names(window & window, XkbDescPtr desc);
	}

	namespace hid
	{
		extern void found_device(int id, int vendor, int product);
		extern void lost_device(int id);

		extern void add_source(int id, const char * path, int type, const char * name);
		extern void remove_source(int id, const char * path);

		extern void dispatch(const Input & input);
	}
}

namespace
{
	using Button = engine::hid::Input::Button;

	Button keycode_to_button[256] = {};

	Button get_button(const char (& name)[XkbKeyNameLength])
	{
		auto begin = std::begin(name);
		auto end = std::find(begin, std::end(name), '\0');
		const engine::Asset asset(begin, end - begin);

		// names taken from
		//
		//  /usr/share/X11/xkb/keycodes/evdev
		//
		// this mapping have to be generated for each client as the x11
		// key codes are not globally unique, the names, however, are
		// assumed to be, run
		//
		//  xev
		//
		// to test key presses and get the key codes for them
		switch (asset)
		{
		case engine::Asset("AB01"): return Button::KEY_Z;
		case engine::Asset("AB02"): return Button::KEY_X;
		case engine::Asset("AB03"): return Button::KEY_C;
		case engine::Asset("AB04"): return Button::KEY_V;
		case engine::Asset("AB05"): return Button::KEY_B;
		case engine::Asset("AB06"): return Button::KEY_N;
		case engine::Asset("AB07"): return Button::KEY_M;
		case engine::Asset("AB08"): return Button::KEY_COMMA;
		case engine::Asset("AB09"): return Button::KEY_DOT;
		case engine::Asset("AB10"): return Button::KEY_SLASH;
		// case engine::Asset("AB11"): return Button::KEY_; // ?
		case engine::Asset("AC01"): return Button::KEY_A;
		case engine::Asset("AC02"): return Button::KEY_S;
		case engine::Asset("AC03"): return Button::KEY_D;
		case engine::Asset("AC04"): return Button::KEY_F;
		case engine::Asset("AC05"): return Button::KEY_G;
		case engine::Asset("AC06"): return Button::KEY_H;
		case engine::Asset("AC07"): return Button::KEY_J;
		case engine::Asset("AC08"): return Button::KEY_K;
		case engine::Asset("AC09"): return Button::KEY_L;
		case engine::Asset("AC10"): return Button::KEY_SEMICOLON;
		case engine::Asset("AC11"): return Button::KEY_APOSTROPHE;
		case engine::Asset("AC12"): return Button::KEY_BACKSLASH;
		case engine::Asset("AD01"): return Button::KEY_Q;
		case engine::Asset("AD02"): return Button::KEY_W;
		case engine::Asset("AD03"): return Button::KEY_E;
		case engine::Asset("AD04"): return Button::KEY_R;
		case engine::Asset("AD05"): return Button::KEY_T;
		case engine::Asset("AD06"): return Button::KEY_Y;
		case engine::Asset("AD07"): return Button::KEY_U;
		case engine::Asset("AD08"): return Button::KEY_I;
		case engine::Asset("AD09"): return Button::KEY_O;
		case engine::Asset("AD10"): return Button::KEY_P;
		case engine::Asset("AD11"): return Button::KEY_LEFTBRACE;
		case engine::Asset("AD12"): return Button::KEY_RIGHTBRACE;
		case engine::Asset("AE01"): return Button::KEY_1;
		case engine::Asset("AE02"): return Button::KEY_2;
		case engine::Asset("AE03"): return Button::KEY_3;
		case engine::Asset("AE04"): return Button::KEY_4;
		case engine::Asset("AE05"): return Button::KEY_5;
		case engine::Asset("AE06"): return Button::KEY_6;
		case engine::Asset("AE07"): return Button::KEY_7;
		case engine::Asset("AE08"): return Button::KEY_8;
		case engine::Asset("AE09"): return Button::KEY_9;
		case engine::Asset("AE10"): return Button::KEY_0;
		case engine::Asset("AE11"): return Button::KEY_MINUS;
		case engine::Asset("AE12"): return Button::KEY_EQUAL;
		// case engine::Asset("AE13"): return Button::KEY_YEN; // ?
		// case engine::Asset("AGAI"): return Button::KEY_AGAIN; // ?
		// case engine::Asset("ALGR"): return Button::KEY_RIGHTALT; // dup?
		// case engine::Asset("ALT"): return Button::KEY_LEFTALT; // dup?
		case engine::Asset("BKSL"): return Button::KEY_BACKSLASH; // dup
		case engine::Asset("BKSP"): return Button::KEY_BACKSPACE;
		// case engine::Asset("BRK"): return Button::KEY_; // ?
		case engine::Asset("CAPS"): return Button::KEY_CAPSLOCK;
		case engine::Asset("COMP"): return Button::KEY_COMPOSE;
		// case engine::Asset("COPY"): return Button::KEY_COPY; // ?
		// case engine::Asset("CUT"): return Button::KEY_CUT; // ?
		case engine::Asset("DELE"): return Button::KEY_DELETE;
		case engine::Asset("DOWN"): return Button::KEY_DOWN;
		case engine::Asset("END"): return Button::KEY_END;
		case engine::Asset("ESC"): return Button::KEY_ESC;
		// case engine::Asset("FIND"): return Button::KEY_FIND; // ?
		case engine::Asset("FK01"): return Button::KEY_F1;
		case engine::Asset("FK02"): return Button::KEY_F2;
		case engine::Asset("FK03"): return Button::KEY_F3;
		case engine::Asset("FK04"): return Button::KEY_F4;
		case engine::Asset("FK05"): return Button::KEY_F5;
		case engine::Asset("FK06"): return Button::KEY_F6;
		case engine::Asset("FK07"): return Button::KEY_F7;
		case engine::Asset("FK08"): return Button::KEY_F8;
		case engine::Asset("FK09"): return Button::KEY_F9;
		case engine::Asset("FK10"): return Button::KEY_F10;
		case engine::Asset("FK11"): return Button::KEY_F11;
		case engine::Asset("FK12"): return Button::KEY_F12;
		case engine::Asset("FK13"): return Button::KEY_F13;
		case engine::Asset("FK14"): return Button::KEY_F14;
		case engine::Asset("FK15"): return Button::KEY_F15;
		case engine::Asset("FK16"): return Button::KEY_F16;
		case engine::Asset("FK17"): return Button::KEY_F17;
		case engine::Asset("FK18"): return Button::KEY_F18;
		case engine::Asset("FK19"): return Button::KEY_F19;
		case engine::Asset("FK20"): return Button::KEY_F20;
		case engine::Asset("FK21"): return Button::KEY_F21;
		case engine::Asset("FK22"): return Button::KEY_F22;
		case engine::Asset("FK23"): return Button::KEY_F23;
		case engine::Asset("FK24"): return Button::KEY_F24;
		// case engine::Asset("FRNT"): return Button::KEY_FRONT; // ?
		// case engine::Asset("HELP"): return Button::KEY_HELP; // ?
		// case engine::Asset("HENK"): return Button::KEY_HENKAN; // ?
		// case engine::Asset("HIRA"): return Button::KEY_HIRAGANA; // ?
		// case engine::Asset("HJCV"): return Button::KEY_HANJA; // ?
		// case engine::Asset("HKTG"): return Button::KEY_KATAKANAHIRAGANA; // ?
		// case engine::Asset("HNGL"): return Button::KEY_HANGUL; // KEY_HANGEUL = KEY_HANGUEL?
		case engine::Asset("HOME"): return Button::KEY_HOME;
		// case engine::Asset("HYPR"): return Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("HZTG"): return Button::KEY_ZENKAKUHANKAKU; // ?
		// case engine::Asset("I120"): return Button::KEY_; // ?
		// case engine::Asset("I126"): return Button::KEY_; // ?
		// case engine::Asset("I128"): return Button::KEY_; // ?
		// case engine::Asset("I129"): return Button::KEY_KPCOMMA; // ?
		// case engine::Asset("I147"): return Button::KEY_; // ?
		// case engine::Asset("I148"): return Button::KEY_; // ?
		// case engine::Asset("I149"): return Button::KEY_; // ?
		// case engine::Asset("I150"): return Button::KEY_; // ?
		// case engine::Asset("I151"): return Button::KEY_; // ?
		// case engine::Asset("I152"): return Button::KEY_; // ?
		// case engine::Asset("I153"): return Button::KEY_; // ?
		// case engine::Asset("I154"): return Button::KEY_; // ?
		// case engine::Asset("I155"): return Button::KEY_; // ?
		// case engine::Asset("I156"): return Button::KEY_; // ?
		// case engine::Asset("I157"): return Button::KEY_; // ?
		// case engine::Asset("I158"): return Button::KEY_; // ?
		// case engine::Asset("I159"): return Button::KEY_; // ?
		// case engine::Asset("I160"): return Button::KEY_; // ?
		// case engine::Asset("I161"): return Button::KEY_; // ?
		// case engine::Asset("I162"): return Button::KEY_; // ?
		// case engine::Asset("I163"): return Button::KEY_; // ?
		// case engine::Asset("I164"): return Button::KEY_; // ?
		// case engine::Asset("I165"): return Button::KEY_; // ?
		// case engine::Asset("I166"): return Button::KEY_; // ?
		// case engine::Asset("I167"): return Button::KEY_; // ?
		// case engine::Asset("I168"): return Button::KEY_; // ?
		// case engine::Asset("I169"): return Button::KEY_; // ?
		// case engine::Asset("I170"): return Button::KEY_; // ?
		// case engine::Asset("I171"): return Button::KEY_; // ?
		// case engine::Asset("I172"): return Button::KEY_; // ?
		// case engine::Asset("I173"): return Button::KEY_; // ?
		// case engine::Asset("I174"): return Button::KEY_; // ?
		// case engine::Asset("I175"): return Button::KEY_; // ?
		// case engine::Asset("I176"): return Button::KEY_; // ?
		// case engine::Asset("I177"): return Button::KEY_; // ?
		// case engine::Asset("I178"): return Button::KEY_; // ?
		// case engine::Asset("I179"): return Button::KEY_; // ?
		// case engine::Asset("I180"): return Button::KEY_; // ?
		// case engine::Asset("I181"): return Button::KEY_; // ?
		// case engine::Asset("I182"): return Button::KEY_; // ?
		// case engine::Asset("I183"): return Button::KEY_; // ?
		// case engine::Asset("I184"): return Button::KEY_; // ?
		// case engine::Asset("I185"): return Button::KEY_; // ?
		// case engine::Asset("I186"): return Button::KEY_; // ?
		// case engine::Asset("I187"): return Button::KEY_; // ?
		// case engine::Asset("I188"): return Button::KEY_; // ?
		// case engine::Asset("I189"): return Button::KEY_; // ?
		// case engine::Asset("I190"): return Button::KEY_; // ?
		// case engine::Asset("I208"): return Button::KEY_; // ?
		// case engine::Asset("I209"): return Button::KEY_; // ?
		// case engine::Asset("I210"): return Button::KEY_; // ?
		// case engine::Asset("I211"): return Button::KEY_; // ?
		// case engine::Asset("I212"): return Button::KEY_; // ?
		// case engine::Asset("I213"): return Button::KEY_; // ?
		// case engine::Asset("I214"): return Button::KEY_; // ?
		// case engine::Asset("I215"): return Button::KEY_; // ?
		// case engine::Asset("I216"): return Button::KEY_; // ?
		// case engine::Asset("I217"): return Button::KEY_; // ?
		// case engine::Asset("I218"): return Button::KEY_; // ?
		// case engine::Asset("I219"): return Button::KEY_; // ?
		// case engine::Asset("I220"): return Button::KEY_; // ?
		// case engine::Asset("I221"): return Button::KEY_; // ?
		// case engine::Asset("I222"): return Button::KEY_; // ?
		// case engine::Asset("I223"): return Button::KEY_; // ?
		// case engine::Asset("I224"): return Button::KEY_; // ?
		// case engine::Asset("I225"): return Button::KEY_; // ?
		// case engine::Asset("I226"): return Button::KEY_; // ?
		// case engine::Asset("I227"): return Button::KEY_; // ?
		// case engine::Asset("I228"): return Button::KEY_; // ?
		// case engine::Asset("I229"): return Button::KEY_; // ?
		// case engine::Asset("I230"): return Button::KEY_; // ?
		// case engine::Asset("I231"): return Button::KEY_; // ?
		// case engine::Asset("I232"): return Button::KEY_; // ?
		// case engine::Asset("I233"): return Button::KEY_; // ?
		// case engine::Asset("I234"): return Button::KEY_; // ?
		// case engine::Asset("I235"): return Button::KEY_; // ?
		// case engine::Asset("I236"): return Button::KEY_; // ?
		// case engine::Asset("I237"): return Button::KEY_; // ?
		// case engine::Asset("I238"): return Button::KEY_; // ?
		// case engine::Asset("I239"): return Button::KEY_; // ?
		// case engine::Asset("I240"): return Button::KEY_; // ?
		// case engine::Asset("I241"): return Button::KEY_; // ?
		// case engine::Asset("I242"): return Button::KEY_; // ?
		// case engine::Asset("I243"): return Button::KEY_; // ?
		// case engine::Asset("I244"): return Button::KEY_; // ?
		// case engine::Asset("I245"): return Button::KEY_; // ?
		// case engine::Asset("I246"): return Button::KEY_; // ?
		// case engine::Asset("I247"): return Button::KEY_; // ?
		// case engine::Asset("I248"): return Button::KEY_; // ?
		// case engine::Asset("I249"): return Button::KEY_; // ?
		// case engine::Asset("I250"): return Button::KEY_; // ?
		// case engine::Asset("I251"): return Button::KEY_; // ?
		// case engine::Asset("I252"): return Button::KEY_; // ?
		// case engine::Asset("I253"): return Button::KEY_; // ?
		case engine::Asset("INS"): return Button::KEY_INSERT;
		// case engine::Asset("JPCM"): return Button::KEY_KPJPCOMMA; // ?
		// case engine::Asset("KATA"): return Button::KEY_KATAKANA; // ?
		case engine::Asset("KP0"): return Button::KEY_KP0;
		case engine::Asset("KP1"): return Button::KEY_KP1;
		case engine::Asset("KP2"): return Button::KEY_KP2;
		case engine::Asset("KP3"): return Button::KEY_KP3;
		case engine::Asset("KP4"): return Button::KEY_KP4;
		case engine::Asset("KP5"): return Button::KEY_KP5;
		case engine::Asset("KP6"): return Button::KEY_KP6;
		case engine::Asset("KP7"): return Button::KEY_KP7;
		case engine::Asset("KP8"): return Button::KEY_KP8;
		case engine::Asset("KP9"): return Button::KEY_KP9;
		case engine::Asset("KPAD"): return Button::KEY_KPPLUS;
		case engine::Asset("KPDL"): return Button::KEY_KPDOT;
		case engine::Asset("KPDV"): return Button::KEY_KPSLASH;
		// case engine::Asset("KPEN"): return Button::KEY_KPENTER; // ?
		// case engine::Asset("KPEQ"): return Button::KEY_; // ?
		case engine::Asset("KPMU"): return Button::KEY_KPASTERISK;
		// case engine::Asset("KPPT"): return Button::KEY_KPCOMMA; // dup?
		case engine::Asset("KPSU"): return Button::KEY_KPMINUS;
		case engine::Asset("LALT"): return Button::KEY_LEFTALT;
		case engine::Asset("LCTL"): return Button::KEY_LEFTCTRL;
		case engine::Asset("LEFT"): return Button::KEY_LEFT;
		case engine::Asset("LMTA"): return Button::KEY_LEFTMETA; // dup
		// case engine::Asset("LNFD"): return Button::KEY_LINEFEED; // ?
		case engine::Asset("LFSH"): return Button::KEY_LEFTSHIFT;
		case engine::Asset("LSGT"): return Button::KEY_102ND;
		// case engine::Asset("LVL3"): return Button::KEY_RIGHTALT; // dup
		case engine::Asset("LWIN"): return Button::KEY_LEFTMETA;
		// case engine::Asset("MDSW"): return Button::KEY_RIGHTALT; // dup?
		// case engine::Asset("MENU"): return Button::KEY_COMPOSE; // dup
		// case engine::Asset("META"): return Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("MUHE"): return Button::KEY_MUHENKAN; // ?
		// case engine::Asset("MUTE"): return Button::KEY_MUTE;
		// case engine::Asset("NMLK"): return Button::KEY_NUMLOCK;
		// case engine::Asset("OPEN"): return Button::KEY_OPEN; // ?
		// case engine::Asset("PAST"): return Button::KEY_PASTE; // ?
		case engine::Asset("PAUS"): return Button::KEY_PAUSE;
		case engine::Asset("PGDN"): return Button::KEY_PAGEDOWN;
		case engine::Asset("PGUP"): return Button::KEY_PAGEUP;
		// case engine::Asset("POWR"): return Button::KEY_POWER; // ?
		// case engine::Asset("PROP"): return Button::KEY_PROPS; // ?
		// case engine::Asset("PRSC"): return Button::KEY_SYSRQ;
		case engine::Asset("RALT"): return Button::KEY_RIGHTALT;
		case engine::Asset("RCTL"): return Button::KEY_RIGHTCTRL;
		case engine::Asset("RGHT"): return Button::KEY_RIGHT;
		case engine::Asset("RMTA"): return Button::KEY_RIGHTMETA; // dup
		// case engine::Asset("RO"): return Button::KEY_RO; // ?
		case engine::Asset("RTRN"): return Button::KEY_ENTER;
		case engine::Asset("RTSH"): return Button::KEY_RIGHTSHIFT;
		case engine::Asset("RWIN"): return Button::KEY_RIGHTMETA;
		// case engine::Asset("SCLK"): return Button::KEY_SCROLLLOCK; // ?
		case engine::Asset("SPCE"): return Button::KEY_SPACE;
		// case engine::Asset("STOP"): return Button::KEY_STOP; // ?
		// case engine::Asset("SUPR"): return Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("SYRQ"): return Button::KEY_SYSRQ; // dup
		case engine::Asset("TAB"): return Button::KEY_TAB;
		case engine::Asset("TLDE"): return Button::KEY_GRAVE;
		// case engine::Asset("UNDO"): return Button::KEY_UNDO; // ?
		case engine::Asset("UP"): return Button::KEY_UP;
		// case engine::Asset("VOL+"): return Button::KEY_VOLUMEUP;
		// case engine::Asset("VOL-"): return Button::KEY_VOLUMEDOWN;
		default: return Button::INVALID;
		}
	}
}

// this is a bit of an ugly hack, the reason why we include it here
// (and not at the top) is because the linux naming convention for
// keys clashes with our own, obviously we could solve this in any
// number of ways most of which involve us having to change our code
// :rage: but why should we?
//
// instead, we will use our serialization framework to map the linux
// keys with our own
#include <linux/input.h>

namespace
{
	constexpr auto code_to_button = utl::make_table<KEY_CNT>(
		std::make_pair(BTN_EXTRA, core::value_table<Button>::get("mouse-extra")),
		std::make_pair(BTN_LEFT, core::value_table<Button>::get("mouse-left")),
		std::make_pair(BTN_MIDDLE, core::value_table<Button>::get("mouse-middle")),
		std::make_pair(BTN_RIGHT, core::value_table<Button>::get("mouse-right")),
		std::make_pair(BTN_SIDE, core::value_table<Button>::get("mouse-side")),

		std::make_pair(BTN_EAST, core::value_table<Button>::get("gamepad-east")),
		std::make_pair(BTN_NORTH, core::value_table<Button>::get("gamepad-north")),
		std::make_pair(BTN_SOUTH, core::value_table<Button>::get("gamepad-south")),
		std::make_pair(BTN_WEST, core::value_table<Button>::get("gamepad-west")),
		std::make_pair(BTN_THUMBL, core::value_table<Button>::get("gamepad-thumbl")),
		std::make_pair(BTN_THUMBR, core::value_table<Button>::get("gamepad-thumbr")),
		std::make_pair(BTN_TL, core::value_table<Button>::get("gamepad-tl")),
		std::make_pair(BTN_TL2, core::value_table<Button>::get("gamepad-tl2")),
		std::make_pair(BTN_TR, core::value_table<Button>::get("gamepad-tr")),
		std::make_pair(BTN_TR2, core::value_table<Button>::get("gamepad-tr2")),
		std::make_pair(BTN_MODE, core::value_table<Button>::get("gamepad-mode")),
		std::make_pair(BTN_SELECT, core::value_table<Button>::get("gamepad-select")),
		std::make_pair(BTN_START, core::value_table<Button>::get("gamepad-start")),

		std::make_pair(KEY_0, core::value_table<Button>::get("key-0")),
		std::make_pair(KEY_1, core::value_table<Button>::get("key-1")),
		std::make_pair(KEY_2, core::value_table<Button>::get("key-2")),
		std::make_pair(KEY_3, core::value_table<Button>::get("key-3")),
		std::make_pair(KEY_4, core::value_table<Button>::get("key-4")),
		std::make_pair(KEY_5, core::value_table<Button>::get("key-5")),
		std::make_pair(KEY_6, core::value_table<Button>::get("key-6")),
		std::make_pair(KEY_7, core::value_table<Button>::get("key-7")),
		std::make_pair(KEY_8, core::value_table<Button>::get("key-8")),
		std::make_pair(KEY_9, core::value_table<Button>::get("key-9")),
		std::make_pair(KEY_A, core::value_table<Button>::get("key-a")),
		std::make_pair(KEY_B, core::value_table<Button>::get("key-b")),
		std::make_pair(KEY_C, core::value_table<Button>::get("key-c")),
		std::make_pair(KEY_D, core::value_table<Button>::get("key-d")),
		std::make_pair(KEY_E, core::value_table<Button>::get("key-e")),
		std::make_pair(KEY_F, core::value_table<Button>::get("key-f")),
		std::make_pair(KEY_G, core::value_table<Button>::get("key-g")),
		std::make_pair(KEY_H, core::value_table<Button>::get("key-h")),
		std::make_pair(KEY_I, core::value_table<Button>::get("key-i")),
		std::make_pair(KEY_J, core::value_table<Button>::get("key-j")),
		std::make_pair(KEY_K, core::value_table<Button>::get("key-k")),
		std::make_pair(KEY_L, core::value_table<Button>::get("key-l")),
		std::make_pair(KEY_M, core::value_table<Button>::get("key-m")),
		std::make_pair(KEY_N, core::value_table<Button>::get("key-n")),
		std::make_pair(KEY_O, core::value_table<Button>::get("key-o")),
		std::make_pair(KEY_P, core::value_table<Button>::get("key-p")),
		std::make_pair(KEY_Q, core::value_table<Button>::get("key-q")),
		std::make_pair(KEY_R, core::value_table<Button>::get("key-r")),
		std::make_pair(KEY_S, core::value_table<Button>::get("key-s")),
		std::make_pair(KEY_T, core::value_table<Button>::get("key-t")),
		std::make_pair(KEY_U, core::value_table<Button>::get("key-u")),
		std::make_pair(KEY_V, core::value_table<Button>::get("key-v")),
		std::make_pair(KEY_W, core::value_table<Button>::get("key-w")),
		std::make_pair(KEY_X, core::value_table<Button>::get("key-x")),
		std::make_pair(KEY_Y, core::value_table<Button>::get("key-y")),
		std::make_pair(KEY_Z, core::value_table<Button>::get("key-z")),
		std::make_pair(KEY_102ND, core::value_table<Button>::get("key-102nd")),
		std::make_pair(KEY_APOSTROPHE, core::value_table<Button>::get("key-apostrophe")),
		std::make_pair(KEY_BACKSLASH, core::value_table<Button>::get("key-backslash")),
		std::make_pair(KEY_BACKSPACE, core::value_table<Button>::get("key-backspace")),
		std::make_pair(KEY_BREAK, core::value_table<Button>::get("key-break")),
		std::make_pair(KEY_CAPSLOCK, core::value_table<Button>::get("key-capslock")),
		std::make_pair(KEY_CLEAR, core::value_table<Button>::get("key-clear")),
		std::make_pair(KEY_COMMA, core::value_table<Button>::get("key-comma")),
		std::make_pair(KEY_COMPOSE, core::value_table<Button>::get("key-compose")),
		std::make_pair(KEY_DELETE, core::value_table<Button>::get("key-delete")),
		std::make_pair(KEY_DOT, core::value_table<Button>::get("key-dot")),
		std::make_pair(KEY_DOWN, core::value_table<Button>::get("key-down")),
		std::make_pair(KEY_END, core::value_table<Button>::get("key-end")),
		std::make_pair(KEY_ENTER, core::value_table<Button>::get("key-enter")),
		std::make_pair(KEY_EQUAL, core::value_table<Button>::get("key-equal")),
		std::make_pair(KEY_ESC, core::value_table<Button>::get("key-esc")),
		std::make_pair(KEY_F1, core::value_table<Button>::get("key-f1")),
		std::make_pair(KEY_F2, core::value_table<Button>::get("key-f2")),
		std::make_pair(KEY_F3, core::value_table<Button>::get("key-f3")),
		std::make_pair(KEY_F4, core::value_table<Button>::get("key-f4")),
		std::make_pair(KEY_F5, core::value_table<Button>::get("key-f5")),
		std::make_pair(KEY_F6, core::value_table<Button>::get("key-f6")),
		std::make_pair(KEY_F7, core::value_table<Button>::get("key-f7")),
		std::make_pair(KEY_F8, core::value_table<Button>::get("key-f8")),
		std::make_pair(KEY_F9, core::value_table<Button>::get("key-f9")),
		std::make_pair(KEY_F10, core::value_table<Button>::get("key-f10")),
		std::make_pair(KEY_F11, core::value_table<Button>::get("key-f11")),
		std::make_pair(KEY_F12, core::value_table<Button>::get("key-f12")),
		std::make_pair(KEY_F13, core::value_table<Button>::get("key-f13")),
		std::make_pair(KEY_F14, core::value_table<Button>::get("key-f14")),
		std::make_pair(KEY_F15, core::value_table<Button>::get("key-f15")),
		std::make_pair(KEY_F16, core::value_table<Button>::get("key-f16")),
		std::make_pair(KEY_F17, core::value_table<Button>::get("key-f17")),
		std::make_pair(KEY_F18, core::value_table<Button>::get("key-f18")),
		std::make_pair(KEY_F19, core::value_table<Button>::get("key-f19")),
		std::make_pair(KEY_F20, core::value_table<Button>::get("key-f20")),
		std::make_pair(KEY_F21, core::value_table<Button>::get("key-f21")),
		std::make_pair(KEY_F22, core::value_table<Button>::get("key-f22")),
		std::make_pair(KEY_F23, core::value_table<Button>::get("key-f23")),
		std::make_pair(KEY_F24, core::value_table<Button>::get("key-f24")),
		std::make_pair(KEY_GRAVE, core::value_table<Button>::get("key-grave")),
		std::make_pair(KEY_HOME, core::value_table<Button>::get("key-home")),
		std::make_pair(KEY_INSERT, core::value_table<Button>::get("key-insert")),
		std::make_pair(KEY_KP0, core::value_table<Button>::get("key-kp0")),
		std::make_pair(KEY_KP1, core::value_table<Button>::get("key-kp1")),
		std::make_pair(KEY_KP2, core::value_table<Button>::get("key-kp2")),
		std::make_pair(KEY_KP3, core::value_table<Button>::get("key-kp3")),
		std::make_pair(KEY_KP4, core::value_table<Button>::get("key-kp4")),
		std::make_pair(KEY_KP5, core::value_table<Button>::get("key-kp5")),
		std::make_pair(KEY_KP6, core::value_table<Button>::get("key-kp6")),
		std::make_pair(KEY_KP7, core::value_table<Button>::get("key-kp7")),
		std::make_pair(KEY_KP8, core::value_table<Button>::get("key-kp8")),
		std::make_pair(KEY_KP9, core::value_table<Button>::get("key-kp9")),
		std::make_pair(KEY_KPASTERISK, core::value_table<Button>::get("key-kpasterisk")),
		std::make_pair(KEY_KPDOT, core::value_table<Button>::get("key-kpdot")),
		std::make_pair(KEY_KPENTER, core::value_table<Button>::get("key-kpenter")),
		std::make_pair(KEY_KPMINUS, core::value_table<Button>::get("key-kpminus")),
		std::make_pair(KEY_KPPLUS, core::value_table<Button>::get("key-kpplus")),
		std::make_pair(KEY_KPSLASH, core::value_table<Button>::get("key-kpslash")),
		std::make_pair(KEY_LEFT, core::value_table<Button>::get("key-left")),
		std::make_pair(KEY_LEFTALT, core::value_table<Button>::get("key-leftalt")),
		std::make_pair(KEY_LEFTBRACE, core::value_table<Button>::get("key-leftbrace")),
		std::make_pair(KEY_LEFTCTRL, core::value_table<Button>::get("key-leftctrl")),
		std::make_pair(KEY_LEFTMETA, core::value_table<Button>::get("key-leftmeta")),
		std::make_pair(KEY_LEFTSHIFT, core::value_table<Button>::get("key-leftshift")),
		std::make_pair(KEY_MINUS, core::value_table<Button>::get("key-minus")),
		std::make_pair(KEY_NUMLOCK, core::value_table<Button>::get("key-numlock")),
		std::make_pair(KEY_PAGEDOWN, core::value_table<Button>::get("key-pagedown")),
		std::make_pair(KEY_PAGEUP, core::value_table<Button>::get("key-pageup")),
		std::make_pair(KEY_PAUSE, core::value_table<Button>::get("key-pause")),
		// printscreen missing?
		std::make_pair(KEY_RIGHT, core::value_table<Button>::get("key-right")),
		std::make_pair(KEY_RIGHTALT, core::value_table<Button>::get("key-rightalt")),
		std::make_pair(KEY_RIGHTBRACE, core::value_table<Button>::get("key-rightbrace")),
		std::make_pair(KEY_RIGHTCTRL, core::value_table<Button>::get("key-rightctrl")),
		std::make_pair(KEY_RIGHTMETA, core::value_table<Button>::get("key-rightmeta")),
		std::make_pair(KEY_RIGHTSHIFT, core::value_table<Button>::get("key-rightshift")),
		std::make_pair(KEY_SCROLLLOCK, core::value_table<Button>::get("key-scrolllock")),
		std::make_pair(KEY_SEMICOLON, core::value_table<Button>::get("key-semicolon")),
		std::make_pair(KEY_SLASH, core::value_table<Button>::get("key-slash")),
		std::make_pair(KEY_SPACE, core::value_table<Button>::get("key-space")),
		std::make_pair(KEY_SYSRQ, core::value_table<Button>::get("key-sysrq")),
		std::make_pair(KEY_TAB, core::value_table<Button>::get("key-tab")),
		std::make_pair(KEY_UP, core::value_table<Button>::get("key-up"))
		);
}

#define n_longs_for(m_bits) (((m_bits) - 1) / (sizeof(unsigned long) * CHAR_BIT) + 1)
#define test_bit(bit, longs) ((longs[(bit) / (sizeof(unsigned long) * CHAR_BIT)] >> ((bit) % (sizeof(unsigned long) * CHAR_BIT))) != 0)

namespace
{
	enum class EventType
	{
		Unknown, // should never be used
		Gamepad,
		Keyboard,
		Mouse,
		Touch,
	};

	constexpr auto serialization(utility::in_place_type_t<EventType>)
	{
		return utility::make_lookup_table(
			std::make_pair(utility::string_view("unknown"), EventType::Unknown),
			std::make_pair(utility::string_view("gamepad"), EventType::Gamepad),
			std::make_pair(utility::string_view("keyboard"), EventType::Keyboard),
			std::make_pair(utility::string_view("mouse"), EventType::Mouse),
			std::make_pair(utility::string_view("touch"), EventType::Touch)
			);
	}

	struct Event
	{
		int fd; //
		EventType type;

		int16_t id;

		struct input_absinfo absinfos[ABS_CNT];

		Event(int fd, EventType type, int16_t id)
			: fd(fd)
			, type(type)
			, id(id)
		{
			debug_assert(type != EventType::Unknown);

			set_absinfos();
		}

		void set_absinfos()
		{
			unsigned long code_bits[n_longs_for(ABS_CNT)] = {};
			::ioctl(fd, EVIOCGBIT(EV_ABS, ABS_CNT), code_bits);
			for (int code = 0; code < ABS_CNT; code++)
			{
				if (test_bit(code, code_bits)) {
					::ioctl(fd, EVIOCGABS(code), &absinfos[code]);
				}
			}
		}
	};

	struct Device
	{
		uint16_t vendor;
		uint16_t product;

		int16_t id;

		int16_t event_count = 0;

		Device(int vendor, int product)
			: vendor(vendor)
			, product(product)
		{
			static int16_t next_id = 1; // 0 is reserved for not hardware input
			id = next_id++;
		}
	};

	bool is_event(const char * name) { return std::strncmp(name, "event", 5) == 0; }

	EventType find_event_type(int fd)
	{
		unsigned long event_bits[n_longs_for(EV_CNT)] = {};
		::ioctl(fd, EVIOCGBIT(EV_SYN, EV_CNT), event_bits);

		unsigned long key_bits[n_longs_for(KEY_CNT)] = {};
		if (test_bit(EV_KEY, event_bits))
		{
			::ioctl(fd, EVIOCGBIT(EV_KEY, KEY_CNT), key_bits);
		}
		unsigned long abs_bits[n_longs_for(ABS_CNT)] = {};
		if (test_bit(EV_ABS, event_bits))
		{
			::ioctl(fd, EVIOCGBIT(EV_ABS, ABS_CNT), abs_bits);
		}

		if (test_bit(BTN_GAMEPAD, key_bits))
			return EventType::Gamepad;

		if (test_bit(ABS_MT_SLOT, abs_bits))
			return EventType::Touch;

		if (test_bit(KEY_ESC, key_bits))
			return EventType::Keyboard;

		if (test_bit(BTN_MOUSE, key_bits))
			return EventType::Mouse;

		return EventType::Unknown;
	}

	void print_info(int fd)
	{
		int driver_version = 0;
		::ioctl(fd, EVIOCGVERSION, &driver_version);
		debug_printline("  driver version: ", driver_version, "(", driver_version >> 16, ".", (driver_version >> 8) & 0xff, ".", driver_version & 0xff, ")");

		struct input_id event_id = {};
		::ioctl(fd, EVIOCGID, &event_id);
		debug_printline("  event id: bustype ", event_id.bustype, " vendor ", event_id.vendor, " product ", event_id.product, " version ", event_id.version);

		char event_name[256] = "Unknown";
		::ioctl(fd, EVIOCGNAME(sizeof event_name), event_name);
		debug_printline("  event name: ", event_name);

		char physical_location[255] = "Unknown";
		::ioctl(fd, EVIOCGPHYS(sizeof physical_location), physical_location);
		debug_printline("  physical location: ", physical_location);

		char unique_identifier[255] = "Unknown";
		::ioctl(fd, EVIOCGUNIQ(sizeof unique_identifier), unique_identifier);
		debug_printline("  unique identifier: ", unique_identifier);

		char event_properties[255] = "Unknown";
		::ioctl(fd, EVIOCGPROP(sizeof event_properties), event_properties);
		debug_printline("  event properties: ", event_properties);

		unsigned long event_bits[n_longs_for(EV_CNT)] = {};
		::ioctl(fd, EVIOCGBIT(EV_SYN, EV_CNT), event_bits);

		int nkeys = 0;
		if (test_bit(EV_KEY, event_bits))
		{
			unsigned long code_bits[n_longs_for(KEY_CNT)] = {};
			::ioctl(fd, EVIOCGBIT(EV_KEY, KEY_CNT), code_bits);

			for (int code = 0; code < KEY_CNT; code++)
			{
				if (test_bit(code, code_bits)) {
					nkeys++;
				}
			}

			debug_printline("  event KEY: ", nkeys, " keys");
		}
		int nrels = 0;
		if (test_bit(EV_REL, event_bits))
		{
			unsigned long code_bits[n_longs_for(REL_CNT)] = {};
			::ioctl(fd, EVIOCGBIT(EV_REL, REL_CNT), code_bits);

			int count = 0;
			for (int code = 0; code < REL_CNT; code++)
			{
				if (test_bit(code, code_bits)) {
					nrels++;
				}
			}

			debug_printline("  event REL: ", nrels, " relative axes");
		}
		int nabs = 0;
		if (test_bit(EV_ABS, event_bits))
		{
			unsigned long code_bits[n_longs_for(ABS_CNT)] = {};
			::ioctl(fd, EVIOCGBIT(EV_ABS, ABS_CNT), code_bits);

			int count = 0;
			for (int code = 0; code < ABS_CNT; code++)
			{
				if (test_bit(code, code_bits)) {
					nabs++;
				}
			}

			debug_printline("  event ABS: ", nabs, " absolute axes");
		}
	}

	enum class EventStatus
	{
		Failed,
		Open,
		Uninteresting,
	};

	struct EventInfo
	{
		std::string path;
		EventStatus status;
		int16_t id;
	};

	Device & get_or_create_device(std::vector<Device> & devices, int vendor, int product)
	{
		for (auto & device : devices)
		{
			if (device.vendor == vendor &&
			    device.product == product)
				return device;
		}
		devices.emplace_back(vendor, product);
		engine::hid::found_device(devices.back().id, vendor, product);
		return devices.back();
	}

	std::pair<EventStatus, int16_t> try_open_event(const char * path, std::vector<Device> & devices, std::vector<Event> & events)
	{
		const int fd = ::open(path, O_RDONLY);
		if (fd < 0)
		{
			debug_printline("event ", path, " open failed with ", errno);

			return {EventStatus::Failed, 0};
		}
		else
		{
			EventType type = find_event_type(fd);
			if (type != EventType::Unknown)
			{
				const auto type_name = core::value_table<EventType>::get_key(type);
				debug_printline("event ", path, "(", type_name, "):");
				print_info(fd);

				struct input_id event_id = {};
				::ioctl(fd, EVIOCGID, &event_id);

				Device & device = get_or_create_device(devices, event_id.vendor, event_id.product);
				events.emplace_back(fd, type, device.id);
				device.event_count++;

				char event_name[256] = "Unknown";
				::ioctl(fd, EVIOCGNAME(sizeof event_name), event_name);

				engine::hid::add_source(device.id, path, static_cast<int>(type), event_name);

				return {EventStatus::Open, device.id};
			}
			else
			{
				debug_printline("event ", path, " is uninteresting");

				::close(fd);

				return {EventStatus::Uninteresting, 0};
			}
		}
	}

	void close_event(const EventInfo & event_info, std::vector<Device> & devices, std::vector<Event> & events)
	{
		debug_assert(event_info.status == EventStatus::Open);

		auto event = std::find_if(events.begin(), events.end(), [& event_info](const Event & event){ return event.id == event_info.id; });
		debug_assert(event != events.end());

		auto device = std::find_if(devices.begin(), devices.end(), [& event_info](const Device & device){ return device.id == event_info.id; });
		debug_assert(device != devices.end());

		::close(event->fd);

		engine::hid::remove_source(device->id, event_info.path.c_str());

		device->event_count--;
		events.erase(event);

		if (device->event_count <= 0)
		{
			engine::hid::lost_device(device->id);
			devices.erase(device);
		}
	}

	void scan_events(std::vector<EventInfo> & event_infos, std::vector<Device> & devices, std::vector<Event> & events)
	{
		struct dirent ** namelist;
		const int n = scandir("/dev/input", &namelist, [](const struct dirent * f){ return is_event(f->d_name) ? 1 : 0; }, nullptr);
		if (n < 0)
		{
			debug_fail("scandir failed with ", errno);
			return;
		}

		for (int i = 0; i < n; i++)
		{
			std::string name = utility::to_string("/dev/input/", namelist[i]->d_name);

			auto it = std::find_if(event_infos.begin(), event_infos.end(), [& name](const EventInfo & i){ return i.path == name; });
			if (it == event_infos.end())
			{
				const auto state = try_open_event(name.c_str(), devices, events);
				event_infos.push_back({std::move(name), state.first, state.second});
			}
			else if (it->status == EventStatus::Failed)
			{
				debug_assert(it->id == 0);

				const auto state = try_open_event(name.c_str(), devices, events);
				it->status = state.first;
				it->id = state.second;
			}
			::free(namelist[i]);
		}
		::free(namelist);
	}

	core::async::Thread thread;
	int interupt_pipe[2];

	void event_watch()
	{
		const int notify_fd = inotify_init1(IN_NONBLOCK);
		if (notify_fd < 0)
		{
			debug_fail("inotify failed with ", errno);
			return;
		}

		const int dev_input_fd = inotify_add_watch(notify_fd, "/dev/input", IN_CREATE | IN_DELETE | IN_ATTRIB);
		// IN_ATTRIB is necessary due to permissions not being set at
		// the time we receive the create notification
		//
		// https://stackoverflow.com/questions/25381103/cant-open-dev-input-js-file-descriptor-after-inotify-event
		if (dev_input_fd < 0)
		{
			debug_fail("inotify_add_watch failed with ", errno);
			close(notify_fd);
			return;
		}

		struct pollfd fds[11] = { // arbitrary
			{interupt_pipe[0], 0, },
			{notify_fd, POLLIN, },
		};

		std::vector<EventInfo> event_infos;
		std::vector<Device> devices;
		std::vector<Event> events;
		scan_events(event_infos, devices, events);

		while (true)
		{
			debug_assert(events.size() <= sizeof fds / sizeof fds[0]);
			for (int i = 0; i < events.size(); i++)
			{
				fds[2 + i].fd = events[i].fd;
				fds[2 + i].events = POLLIN;
			}

			if (poll(fds, 2 + events.size(), -1) < 0)
			{
				debug_assert(errno != EFAULT);
				debug_assert(errno != EINVAL);
				debug_assert(errno != ENOMEM);
				debug_assert(errno == EINTR);
				continue;
			}

			debug_assert(!(fds[0].revents & (POLLERR | POLLNVAL)));
			if (fds[0].revents & POLLHUP)
				break;

			for (int i = 0; i < events.size(); i++)
			{
				if (fds[2 + i].revents & POLLIN)
				{
					struct input_event input[20]; // arbitrary
					const int n = ::read(fds[2 + i].fd, input, sizeof input);
					// "[...] you’ll always get a whole number of input
					// events on a read."
					//
					// https://www.kernel.org/doc/html/latest/input/input.html#event-interface
					debug_assert(n > 0);

					for (int j = 0; j < n / sizeof input[0]; j++)
					{
						switch (input[j].type)
						{
						case EV_KEY:
						{
							const auto button = code_to_button[input[j].code];
							if (button != Button::INVALID)
							{
								engine::hid::dispatch(engine::hid::ButtonStateInput(events[i].id, button, input[j].value != 0));
							}
							break;
						}
						case EV_ABS:
							switch (events[i].type)
							{
							case EventType::Gamepad:
							{
								const int64_t diff = int64_t(input[j].value) - int64_t(events[i].absinfos[input[j].code].value);
								int value;
								if (std::abs(diff) < events[i].absinfos[input[j].code].flat)
								{
									value = 0;
								}
								else
								{
									value = input[j].value;
									value = std::min(value, events[i].absinfos[input[j].code].maximum);
									value = std::max(value, events[i].absinfos[input[j].code].minimum);
									value = core::maths::interpolate_and_scale(events[i].absinfos[input[j].code].minimum, events[i].absinfos[input[j].code].maximum, value);
								}

								switch (input[j].code)
								{
								case ABS_X: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_STICKL_X, value)); break;
								case ABS_Y: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_STICKL_Y, value)); break;
								case ABS_Z: engine::hid::dispatch(engine::hid::AxisTriggerInput(events[i].id, engine::hid::Input::Axis::TRIGGER_TL2, value)); break;
								case ABS_RX: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_STICKR_X, value)); break;
								case ABS_RY: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_STICKR_Y, value)); break;
								case ABS_RZ: engine::hid::dispatch(engine::hid::AxisTriggerInput(events[i].id, engine::hid::Input::Axis::TRIGGER_TR2, value)); break;
								case ABS_HAT0X: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_DPAD_X, value)); break;
								case ABS_HAT0Y: engine::hid::dispatch(engine::hid::AxisTiltInput(events[i].id, engine::hid::Input::Axis::TILT_DPAD_Y, value)); break;
								}
								break;
							}
							default:
								break;
							}
							break;
						}
					}
				}
			}

			if (fds[1].revents & POLLIN)
			{
				std::aligned_storage_t<(sizeof(struct inotify_event) + NAME_MAX + 1), alignof(struct inotify_event)> buffer;

				const auto size = read(fds[1].fd, &buffer, sizeof buffer);
				if (size < 0)
				{
					debug_fail(errno);
				}

				for (int from = 0; from < size;)
				{
					const struct inotify_event * const event = reinterpret_cast<struct inotify_event*>(reinterpret_cast<char * >(&buffer) + from);
					// todo: this does not look safe with regard to
					// alignment, but it is what the example in the man
					// pages do :worried:
					//
					//  man 7 inotify
					from += sizeof(struct inotify_event) + event->len;

					if (is_event(event->name))
					{
						std::string name = utility::to_string("/dev/input/", event->name);

						if (event->mask & IN_CREATE)
						{
							auto it = std::find_if(event_infos.begin(), event_infos.end(), [& name](const EventInfo & i){ return i.path == name; });
							if (debug_verify(it == event_infos.end(), "congratulations :tada: you found a data race"))
							{
								const auto state = try_open_event(name.c_str(), devices, events);
								event_infos.push_back({std::move(name), state.first, state.second});
							}
						}
						if (event->mask & IN_DELETE)
						{
							debug_printline("event ", event->name, " lost");

							auto it = std::find_if(event_infos.begin(), event_infos.end(), [& name](const EventInfo & i){ return i.path == name; });
							if (debug_verify(it != event_infos.end(), "congratulations :tada: you found a data race"))
							{
								if (it->status == EventStatus::Open)
								{
									close_event(*it, devices, events);
								}
								event_infos.erase(it);
							}
						}
						if (event->mask & IN_ATTRIB)
						{
							auto it = std::find_if(event_infos.begin(), event_infos.end(), [& name](const EventInfo & i){ return i.path == name; });
							debug_assert(it != event_infos.end(), "congratulations :tada: you found a data race");
							if (it != event_infos.end() && it->status == EventStatus::Failed)
							{
								debug_assert(it->id == 0);

								const auto state = try_open_event(name.c_str(), devices, events);
								it->status = state.first;
								it->id = state.second;
							}
						}
					}
				}
			}
		}

		for (int i = 0; i < event_infos.size(); i++)
		{
			if (event_infos[i].status == EventStatus::Open)
			{
				close_event(event_infos[i], devices, events);
			}
		}

		close(notify_fd);

		close(interupt_pipe[0]);
	}

	std::atomic_int hardware_input(0);

	void start_hardware_input()
	{
		debug_assert(hardware_input.load(std::memory_order_relaxed) == -1);

		pipe(interupt_pipe);
		thread = core::async::Thread{ event_watch };
	}

	void stop_hardware_input()
	{
		debug_assert(hardware_input.load(std::memory_order_relaxed) == -1);

		close(interupt_pipe[1]);
		thread.join();
	}

	int lock_state_variable(std::atomic_int & state)
	{
		int value;
		do
		{
			value = state.load(std::memory_order_relaxed); // acquire?
			while (!state.compare_exchange_weak(value, -1, std::memory_order_relaxed)); // ??
		}
		while (value == -1);

		return value;
	}

	void disable_hardware_input()
	{
		if (lock_state_variable(hardware_input) != 0)
		{
			stop_hardware_input();
		}
		hardware_input.store(0, std::memory_order_relaxed); // release?
	}

	void enable_hardware_input()
	{
		if (lock_state_variable(hardware_input) != 1)
		{
			start_hardware_input();
		}
		hardware_input.store(1, std::memory_order_relaxed); // release?
	}

	void disable_hardware_input_callback(void *)
	{
		disable_hardware_input();
	}

	void enable_hardware_input_callback(void *)
	{
		enable_hardware_input();
	}

	void toggle_hardware_input_callback(void *)
	{
		int value = lock_state_variable(hardware_input);
		if (value == 0)
		{
			value = 1;
			start_hardware_input();
		}
		else
		{
			value = 0;
			stop_hardware_input();
		}
		hardware_input.store(value, std::memory_order_relaxed); // release?
	}
}

namespace engine
{
	namespace hid
	{
		void destroy_subsystem(devices & devices)
		{
			disable_hardware_input();

			engine::abandon("toggle-hardware-input");
			engine::abandon("enable-hardware-input");
			engine::abandon("disable-hardware-input");

			lost_device(0); // non hardware device
		}

		void create_subsystem(devices & devices, engine::application::window & window, bool hardware_input)
		{
			if (XkbDescPtr const desc = load_key_names(window))
			{
				for (int code = desc->min_key_code; code < desc->max_key_code; code++)
				{
					keycode_to_button[code] = get_button(desc->names->keys[code].name);
				}

				free_key_names(window, desc);
			}

			found_device(0, 0, 0); // non hardware device

			engine::observe("disable-hardware-input", disable_hardware_input_callback, nullptr);
			engine::observe("enable-hardware-input", enable_hardware_input_callback, nullptr);
			engine::observe("toggle-hardware-input", toggle_hardware_input_callback, nullptr);

			if (hardware_input)
			{
				enable_hardware_input();
			}
		}

		void key_character(devices & devices, XKeyEvent & event, utility::unicode_code_point cp)
		{
			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " character ", cp);
			dispatch(KeyCharacterInput(0, button, cp));
		}

		void button_press(devices & devices, XButtonEvent & event)
		{
			if (hardware_input.load(std::memory_order_relaxed))
				return;

			debug_assert((event.button >= 1 && event.button < 6));
			const engine::hid::Input::Button buttons[] =
				{
					engine::hid::Input::Button::INVALID,
					engine::hid::Input::Button::MOUSE_LEFT,
					engine::hid::Input::Button::MOUSE_RIGHT,
					engine::hid::Input::Button::MOUSE_MIDDLE,
					engine::hid::Input::Button::MOUSE_SIDE, // ?
					engine::hid::Input::Button::MOUSE_EXTRA, // ?
				};

			const engine::hid::Input::Button button = buttons[event.button];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("mouse ", button_name, " down");
			dispatch(ButtonStateInput(0, button, true));
		}

		void button_release(devices & devices, XButtonEvent & event)
		{
			if (hardware_input.load(std::memory_order_relaxed))
				return;

			debug_assert((event.button >= 1 && event.button < 6));
			const engine::hid::Input::Button buttons[] =
				{
					engine::hid::Input::Button::INVALID,
					engine::hid::Input::Button::MOUSE_LEFT,
					engine::hid::Input::Button::MOUSE_RIGHT,
					engine::hid::Input::Button::MOUSE_MIDDLE,
					engine::hid::Input::Button::MOUSE_SIDE, // ?
					engine::hid::Input::Button::MOUSE_EXTRA, // ?
				};

			const engine::hid::Input::Button button = buttons[event.button];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("mouse ", button_name, " down");
			dispatch(ButtonStateInput(0, button, false));
		}

		void key_press(devices & devices, XKeyEvent & event)
		{
			if (hardware_input.load(std::memory_order_relaxed))
				return;

			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " down");
			dispatch(ButtonStateInput(0, button, true));
		}

		void key_release(devices & devices, XKeyEvent & event)
		{
			if (hardware_input.load(std::memory_order_relaxed))
				return;

			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " up");
			dispatch(ButtonStateInput(0, button, false));
		}

		void motion_notify(devices & devices, int x, int y, ::Time time)
		{
			dispatch(CursorMoveInput(0, x, y));
		}
	}
}

#endif /* WINDOW_USE_X11 */
