#include "config.h"

#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/hid/devices.hpp"
#include "engine/hid/ui.hpp"

#include <catch2/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Human input device Devices can be created and destroyed", "[.engine][.hid]")
{
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandleW(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif
	engine::hid::ui ui;

	for (int i = 0; i < 2; i++)
	{
		engine::hid::devices devices(window, ui, false);
	}

	for (int i = 0; i < 2; i++)
	{
		engine::hid::devices devices(window, ui, true);
	}
}
