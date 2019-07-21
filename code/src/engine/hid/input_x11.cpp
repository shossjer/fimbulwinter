
#include <config.h>

#if WINDOW_USE_X11

#include "input.hpp"

#include "core/async/Thread.hpp"

#include "engine/Asset.hpp"
#include "engine/debug.hpp"

#include "utility/unicode.hpp"

#include <climits>
#include <vector>

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
		namespace window
		{
			extern XkbDescPtr load_key_names();
			extern void free_key_names(XkbDescPtr desc);
		}
	}

	namespace hid
	{
		extern void dispatch(const Input & input);
	}
}

namespace
{
	engine::hid::Input input;

	static_assert(static_cast<int>(engine::hid::Input::Button::INVALID) == 0, "We rely on that the table gets defaulted to invalid buttons");
	std::array<engine::hid::Input::Button, 256> keycode_to_button = {};

	engine::hid::Input::Button get_button(const char (& name)[XkbKeyNameLength])
	{
		auto begin = std::begin(name);
		auto end = std::find(begin, std::end(name), '\0');
		const engine::Asset asset(begin, end - begin);

		// names taken from
		// /usr/share/X11/xkb/keycodes/evdev
		// this mapping have to be generated for each client as the x11 key codes
		// are not globally unique, the names, however, are assumed to be, run
		// xev
		// to test key presses and get the key codes for them
		switch (asset)
		{
		case engine::Asset("AB01"): return engine::hid::Input::Button::KEY_Z;
		case engine::Asset("AB02"): return engine::hid::Input::Button::KEY_X;
		case engine::Asset("AB03"): return engine::hid::Input::Button::KEY_C;
		case engine::Asset("AB04"): return engine::hid::Input::Button::KEY_V;
		case engine::Asset("AB05"): return engine::hid::Input::Button::KEY_B;
		case engine::Asset("AB06"): return engine::hid::Input::Button::KEY_N;
		case engine::Asset("AB07"): return engine::hid::Input::Button::KEY_M;
		case engine::Asset("AB08"): return engine::hid::Input::Button::KEY_COMMA;
		case engine::Asset("AB09"): return engine::hid::Input::Button::KEY_DOT;
		case engine::Asset("AB10"): return engine::hid::Input::Button::KEY_SLASH;
		// case engine::Asset("AB11"): return engine::hid::Input::Button::KEY_; // ?
		case engine::Asset("AC01"): return engine::hid::Input::Button::KEY_A;
		case engine::Asset("AC02"): return engine::hid::Input::Button::KEY_S;
		case engine::Asset("AC03"): return engine::hid::Input::Button::KEY_D;
		case engine::Asset("AC04"): return engine::hid::Input::Button::KEY_F;
		case engine::Asset("AC05"): return engine::hid::Input::Button::KEY_G;
		case engine::Asset("AC06"): return engine::hid::Input::Button::KEY_H;
		case engine::Asset("AC07"): return engine::hid::Input::Button::KEY_J;
		case engine::Asset("AC08"): return engine::hid::Input::Button::KEY_K;
		case engine::Asset("AC09"): return engine::hid::Input::Button::KEY_L;
		case engine::Asset("AC10"): return engine::hid::Input::Button::KEY_SEMICOLON;
		case engine::Asset("AC11"): return engine::hid::Input::Button::KEY_APOSTROPHE;
		case engine::Asset("AC12"): return engine::hid::Input::Button::KEY_BACKSLASH;
		case engine::Asset("AD01"): return engine::hid::Input::Button::KEY_Q;
		case engine::Asset("AD02"): return engine::hid::Input::Button::KEY_W;
		case engine::Asset("AD03"): return engine::hid::Input::Button::KEY_E;
		case engine::Asset("AD04"): return engine::hid::Input::Button::KEY_R;
		case engine::Asset("AD05"): return engine::hid::Input::Button::KEY_T;
		case engine::Asset("AD06"): return engine::hid::Input::Button::KEY_Y;
		case engine::Asset("AD07"): return engine::hid::Input::Button::KEY_U;
		case engine::Asset("AD08"): return engine::hid::Input::Button::KEY_I;
		case engine::Asset("AD09"): return engine::hid::Input::Button::KEY_O;
		case engine::Asset("AD10"): return engine::hid::Input::Button::KEY_P;
		case engine::Asset("AD11"): return engine::hid::Input::Button::KEY_LEFTBRACE;
		case engine::Asset("AD12"): return engine::hid::Input::Button::KEY_RIGHTBRACE;
		case engine::Asset("AE01"): return engine::hid::Input::Button::KEY_1;
		case engine::Asset("AE02"): return engine::hid::Input::Button::KEY_2;
		case engine::Asset("AE03"): return engine::hid::Input::Button::KEY_3;
		case engine::Asset("AE04"): return engine::hid::Input::Button::KEY_4;
		case engine::Asset("AE05"): return engine::hid::Input::Button::KEY_5;
		case engine::Asset("AE06"): return engine::hid::Input::Button::KEY_6;
		case engine::Asset("AE07"): return engine::hid::Input::Button::KEY_7;
		case engine::Asset("AE08"): return engine::hid::Input::Button::KEY_8;
		case engine::Asset("AE09"): return engine::hid::Input::Button::KEY_9;
		case engine::Asset("AE10"): return engine::hid::Input::Button::KEY_0;
		case engine::Asset("AE11"): return engine::hid::Input::Button::KEY_MINUS;
		case engine::Asset("AE12"): return engine::hid::Input::Button::KEY_EQUAL;
		// case engine::Asset("AE13"): return engine::hid::Input::Button::KEY_YEN; // ?
		// case engine::Asset("AGAI"): return engine::hid::Input::Button::KEY_AGAIN; // ?
		// case engine::Asset("ALGR"): return engine::hid::Input::Button::KEY_RIGHTALT; // dup?
		// case engine::Asset("ALT"): return engine::hid::Input::Button::KEY_LEFTALT; // dup?
		case engine::Asset("BKSL"): return engine::hid::Input::Button::KEY_BACKSLASH; // dup
		case engine::Asset("BKSP"): return engine::hid::Input::Button::KEY_BACKSPACE;
		// case engine::Asset("BRK"): return engine::hid::Input::Button::KEY_; // ?
		case engine::Asset("CAPS"): return engine::hid::Input::Button::KEY_CAPSLOCK;
		case engine::Asset("COMP"): return engine::hid::Input::Button::KEY_COMPOSE;
		// case engine::Asset("COPY"): return engine::hid::Input::Button::KEY_COPY; // ?
		// case engine::Asset("CUT"): return engine::hid::Input::Button::KEY_CUT; // ?
		case engine::Asset("DELE"): return engine::hid::Input::Button::KEY_DELETE;
		case engine::Asset("DOWN"): return engine::hid::Input::Button::KEY_DOWN;
		case engine::Asset("END"): return engine::hid::Input::Button::KEY_END;
		case engine::Asset("ESC"): return engine::hid::Input::Button::KEY_ESC;
		// case engine::Asset("FIND"): return engine::hid::Input::Button::KEY_FIND; // ?
		case engine::Asset("FK01"): return engine::hid::Input::Button::KEY_F1;
		case engine::Asset("FK02"): return engine::hid::Input::Button::KEY_F2;
		case engine::Asset("FK03"): return engine::hid::Input::Button::KEY_F3;
		case engine::Asset("FK04"): return engine::hid::Input::Button::KEY_F4;
		case engine::Asset("FK05"): return engine::hid::Input::Button::KEY_F5;
		case engine::Asset("FK06"): return engine::hid::Input::Button::KEY_F6;
		case engine::Asset("FK07"): return engine::hid::Input::Button::KEY_F7;
		case engine::Asset("FK08"): return engine::hid::Input::Button::KEY_F8;
		case engine::Asset("FK09"): return engine::hid::Input::Button::KEY_F9;
		case engine::Asset("FK10"): return engine::hid::Input::Button::KEY_F10;
		case engine::Asset("FK11"): return engine::hid::Input::Button::KEY_F11;
		case engine::Asset("FK12"): return engine::hid::Input::Button::KEY_F12;
		case engine::Asset("FK13"): return engine::hid::Input::Button::KEY_F13;
		case engine::Asset("FK14"): return engine::hid::Input::Button::KEY_F14;
		case engine::Asset("FK15"): return engine::hid::Input::Button::KEY_F15;
		case engine::Asset("FK16"): return engine::hid::Input::Button::KEY_F16;
		case engine::Asset("FK17"): return engine::hid::Input::Button::KEY_F17;
		case engine::Asset("FK18"): return engine::hid::Input::Button::KEY_F18;
		case engine::Asset("FK19"): return engine::hid::Input::Button::KEY_F19;
		case engine::Asset("FK20"): return engine::hid::Input::Button::KEY_F20;
		case engine::Asset("FK21"): return engine::hid::Input::Button::KEY_F21;
		case engine::Asset("FK22"): return engine::hid::Input::Button::KEY_F22;
		case engine::Asset("FK23"): return engine::hid::Input::Button::KEY_F23;
		case engine::Asset("FK24"): return engine::hid::Input::Button::KEY_F24;
		// case engine::Asset("FRNT"): return engine::hid::Input::Button::KEY_FRONT; // ?
		// case engine::Asset("HELP"): return engine::hid::Input::Button::KEY_HELP; // ?
		// case engine::Asset("HENK"): return engine::hid::Input::Button::KEY_HENKAN; // ?
		// case engine::Asset("HIRA"): return engine::hid::Input::Button::KEY_HIRAGANA; // ?
		// case engine::Asset("HJCV"): return engine::hid::Input::Button::KEY_HANJA; // ?
		// case engine::Asset("HKTG"): return engine::hid::Input::Button::KEY_KATAKANAHIRAGANA; // ?
		// case engine::Asset("HNGL"): return engine::hid::Input::Button::KEY_HANGUL; // KEY_HANGEUL = KEY_HANGUEL?
		case engine::Asset("HOME"): return engine::hid::Input::Button::KEY_HOME;
		// case engine::Asset("HYPR"): return engine::hid::Input::Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("HZTG"): return engine::hid::Input::Button::KEY_ZENKAKUHANKAKU; // ?
		// case engine::Asset("I120"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I126"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I128"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I129"): return engine::hid::Input::Button::KEY_KPCOMMA; // ?
		// case engine::Asset("I147"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I148"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I149"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I150"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I151"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I152"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I153"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I154"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I155"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I156"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I157"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I158"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I159"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I160"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I161"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I162"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I163"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I164"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I165"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I166"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I167"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I168"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I169"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I170"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I171"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I172"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I173"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I174"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I175"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I176"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I177"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I178"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I179"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I180"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I181"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I182"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I183"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I184"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I185"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I186"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I187"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I188"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I189"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I190"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I208"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I209"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I210"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I211"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I212"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I213"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I214"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I215"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I216"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I217"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I218"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I219"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I220"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I221"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I222"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I223"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I224"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I225"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I226"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I227"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I228"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I229"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I230"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I231"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I232"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I233"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I234"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I235"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I236"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I237"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I238"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I239"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I240"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I241"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I242"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I243"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I244"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I245"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I246"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I247"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I248"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I249"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I250"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I251"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I252"): return engine::hid::Input::Button::KEY_; // ?
		// case engine::Asset("I253"): return engine::hid::Input::Button::KEY_; // ?
		case engine::Asset("INS"): return engine::hid::Input::Button::KEY_INSERT;
		// case engine::Asset("JPCM"): return engine::hid::Input::Button::KEY_KPJPCOMMA; // ?
		// case engine::Asset("KATA"): return engine::hid::Input::Button::KEY_KATAKANA; // ?
		case engine::Asset("KP0"): return engine::hid::Input::Button::KEY_KP0;
		case engine::Asset("KP1"): return engine::hid::Input::Button::KEY_KP1;
		case engine::Asset("KP2"): return engine::hid::Input::Button::KEY_KP2;
		case engine::Asset("KP3"): return engine::hid::Input::Button::KEY_KP3;
		case engine::Asset("KP4"): return engine::hid::Input::Button::KEY_KP4;
		case engine::Asset("KP5"): return engine::hid::Input::Button::KEY_KP5;
		case engine::Asset("KP6"): return engine::hid::Input::Button::KEY_KP6;
		case engine::Asset("KP7"): return engine::hid::Input::Button::KEY_KP7;
		case engine::Asset("KP8"): return engine::hid::Input::Button::KEY_KP8;
		case engine::Asset("KP9"): return engine::hid::Input::Button::KEY_KP9;
		case engine::Asset("KPAD"): return engine::hid::Input::Button::KEY_KPPLUS;
		case engine::Asset("KPDL"): return engine::hid::Input::Button::KEY_KPDOT;
		case engine::Asset("KPDV"): return engine::hid::Input::Button::KEY_KPSLASH;
		// case engine::Asset("KPEN"): return engine::hid::Input::Button::KEY_KPENTER; // ?
		// case engine::Asset("KPEQ"): return engine::hid::Input::Button::KEY_; // ?
		case engine::Asset("KPMU"): return engine::hid::Input::Button::KEY_KPASTERISK;
		// case engine::Asset("KPPT"): return engine::hid::Input::Button::KEY_KPCOMMA; // dup?
		case engine::Asset("KPSU"): return engine::hid::Input::Button::KEY_KPMINUS;
		case engine::Asset("LALT"): return engine::hid::Input::Button::KEY_LEFTALT;
		case engine::Asset("LCTL"): return engine::hid::Input::Button::KEY_LEFTCTRL;
		case engine::Asset("LEFT"): return engine::hid::Input::Button::KEY_LEFT;
		case engine::Asset("LMTA"): return engine::hid::Input::Button::KEY_LEFTMETA; // dup
		// case engine::Asset("LNFD"): return engine::hid::Input::Button::KEY_LINEFEED; // ?
		case engine::Asset("LFSH"): return engine::hid::Input::Button::KEY_LEFTSHIFT;
		case engine::Asset("LSGT"): return engine::hid::Input::Button::KEY_102ND;
		// case engine::Asset("LVL3"): return engine::hid::Input::Button::KEY_RIGHTALT; // dup
		case engine::Asset("LWIN"): return engine::hid::Input::Button::KEY_LEFTMETA;
		// case engine::Asset("MDSW"): return engine::hid::Input::Button::KEY_RIGHTALT; // dup?
		// case engine::Asset("MENU"): return engine::hid::Input::Button::KEY_COMPOSE; // dup
		// case engine::Asset("META"): return engine::hid::Input::Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("MUHE"): return engine::hid::Input::Button::KEY_MUHENKAN; // ?
		// case engine::Asset("MUTE"): return engine::hid::Input::Button::KEY_MUTE;
		// case engine::Asset("NMLK"): return engine::hid::Input::Button::KEY_NUMLOCK;
		// case engine::Asset("OPEN"): return engine::hid::Input::Button::KEY_OPEN; // ?
		// case engine::Asset("PAST"): return engine::hid::Input::Button::KEY_PASTE; // ?
		case engine::Asset("PAUS"): return engine::hid::Input::Button::KEY_PAUSE;
		case engine::Asset("PGDN"): return engine::hid::Input::Button::KEY_PAGEDOWN;
		case engine::Asset("PGUP"): return engine::hid::Input::Button::KEY_PAGEUP;
		// case engine::Asset("POWR"): return engine::hid::Input::Button::KEY_POWER; // ?
		// case engine::Asset("PROP"): return engine::hid::Input::Button::KEY_PROPS; // ?
		// case engine::Asset("PRSC"): return engine::hid::Input::Button::KEY_SYSRQ;
		case engine::Asset("RALT"): return engine::hid::Input::Button::KEY_RIGHTALT;
		case engine::Asset("RCTL"): return engine::hid::Input::Button::KEY_RIGHTCTRL;
		case engine::Asset("RGHT"): return engine::hid::Input::Button::KEY_RIGHT;
		case engine::Asset("RMTA"): return engine::hid::Input::Button::KEY_RIGHTMETA; // dup
		// case engine::Asset("RO"): return engine::hid::Input::Button::KEY_RO; // ?
		case engine::Asset("RTRN"): return engine::hid::Input::Button::KEY_ENTER;
		case engine::Asset("RTSH"): return engine::hid::Input::Button::KEY_RIGHTSHIFT;
		case engine::Asset("RWIN"): return engine::hid::Input::Button::KEY_RIGHTMETA;
		// case engine::Asset("SCLK"): return engine::hid::Input::Button::KEY_SCROLLLOCK; // ?
		case engine::Asset("SPCE"): return engine::hid::Input::Button::KEY_SPACE;
		// case engine::Asset("STOP"): return engine::hid::Input::Button::KEY_STOP; // ?
		// case engine::Asset("SUPR"): return engine::hid::Input::Button::KEY_LEFTMETA; // dup?
		// case engine::Asset("SYRQ"): return engine::hid::Input::Button::KEY_SYSRQ; // dup
		case engine::Asset("TAB"): return engine::hid::Input::Button::KEY_TAB;
		case engine::Asset("TLDE"): return engine::hid::Input::Button::KEY_GRAVE;
		// case engine::Asset("UNDO"): return engine::hid::Input::Button::KEY_UNDO; // ?
		case engine::Asset("UP"): return engine::hid::Input::Button::KEY_UP;
		// case engine::Asset("VOL+"): return engine::hid::Input::Button::KEY_VOLUMEUP;
		// case engine::Asset("VOL-"): return engine::hid::Input::Button::KEY_VOLUMEDOWN;
		default: return engine::hid::Input::Button::KEY_0;
		}
	}

	bool is_event(const char * name) { return std::strncmp(name, "event", 5) == 0; }

	std::vector<std::string> failed_devices;

	core::async::Thread thread;
	int interupt_pipe[2];

	void device_watch()
	{
		const int notify_fd = inotify_init1(IN_NONBLOCK);
		if (notify_fd < 0)
		{
			debug_fail("inotify failed with ", errno);
			return;
		}

		const int dev_input_fd = inotify_add_watch(notify_fd, "/dev/input", IN_CREATE | IN_DELETE | IN_ATTRIB);
		// IN_ATTRIB is necessary due to permissions not being set at the time we
		// receive the create notification, see
		// https://stackoverflow.com/questions/25381103/cant-open-dev-input-js-file-descriptor-after-inotify-event
		if (dev_input_fd < 0)
		{
			debug_fail("inotify_add_watch failed with ", errno);
			close(notify_fd);
			return;
		}

		struct pollfd fds[2] = {
			{interupt_pipe[0], 0, 0},
			{notify_fd, POLLIN, 0}
		};

		while (true)
		{
			if (poll(fds, sizeof fds / sizeof fds[0], -1) < 0)
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
					// todo: this does not look safe, but it is what the
					// example in the man pages do :worried:
					// man 7 inotify
					from += sizeof(struct inotify_event) + event->len;

					if (event->mask & IN_CREATE)
					{
						if (is_event(event->name))
						{
							debug_printline("device ", event->name, " detected");

							const std::string name = utility::to_string("/dev/input/", event->name);
							const int fd = ::open(name.c_str(), O_RDONLY);
							if (fd < 0)
							{
								debug_printline("open failed with ", errno);
								if (std::find(failed_devices.begin(), failed_devices.end(), name) == failed_devices.end())
								{
									failed_devices.push_back(std::move(name));
								}
							}
							else
							{
								debug_printline("device ", event->name, ":");

								::close(fd);
							}
						}
					}
					if (event->mask & IN_DELETE)
					{
						if (is_event(event->name))
						{
							debug_printline("device ", event->name, " lost");

							const std::string name = utility::to_string("/dev/input/", event->name);
							auto it = std::find(failed_devices.begin(), failed_devices.end(), name);
							if (it != failed_devices.end())
							{
								failed_devices.erase(it);
							}
						}
					}
					if (event->mask & IN_ATTRIB)
					{
						if (is_event(event->name))
						{
							const std::string name = utility::to_string("/dev/input/", event->name);
							auto it = std::find(failed_devices.begin(), failed_devices.end(), name);
							if (it != failed_devices.end())
							{
								debug_printline("device ", event->name, " changed, trying again");

								const int fd = ::open(name.c_str(), O_RDONLY);
								if (fd < 0)
								{
									debug_printline("open failed with ", errno);
								}
								else
								{
									failed_devices.erase(it);

									debug_printline("device ", event->name, ":");

									::close(fd);
								}
							}
						}
					}
				}
			}
		}

		close(notify_fd);

		close(interupt_pipe[0]);
	}
}

namespace engine
{
	namespace hid
	{
		void create()
		{
			pipe(interupt_pipe);
			thread = core::async::Thread{ device_watch };

			if (XkbDescPtr const desc = engine::application::window::load_key_names())
			{
				for (int code = desc->min_key_code; code < desc->max_key_code; code++)
				{
					keycode_to_button[code] = get_button(desc->names->keys[code].name);
				}

				engine::application::window::free_key_names(desc);
			}
		}

		void destroy()
		{
			close(interupt_pipe[1]);
			thread.join();
		}

		void key_character(XKeyEvent & event, utility::code_point cp)
		{
			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " character ", cp);
			input.setKeyCharacter(0, button, cp);
			dispatch(input);
		}

		void button_press(XButtonEvent & event)
		{
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
			input.setButtonDown(0, button, event.x, event.y);
			dispatch(input);
		}
		void button_release(XButtonEvent & event)
		{
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
			input.setButtonUp(0, button, event.x, event.y);
			dispatch(input);
		}
		void key_press(XKeyEvent & event)
		{
			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " down");
			input.setButtonDown(0, button, event.x, event.y);
			dispatch(input);
		}
		void key_release(XKeyEvent & event)
		{
			const engine::hid::Input::Button button = keycode_to_button[event.keycode];
			// const auto button_name = core::value_table<engine::hid::Input::Button>::get_key(button);
			// debug_printline("key ", button_name, " up");
			input.setButtonUp(0, button, event.x, event.y);
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
