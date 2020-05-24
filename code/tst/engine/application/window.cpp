#include "config.h"

#include "engine/application/config.hpp"
#include "engine/application/window.hpp"

#include <catch2/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Application window can be created and destroyed", "[.engine][.application]")
{
	engine::application::config_t config;

	for (int i = 0; i < 2; i++)
	{
#if WINDOW_USE_USER32
		engine::application::window window(GetModuleHandleW(nullptr), SW_HIDE, config);
#else
		engine::application::window window(config);
#endif
	}
}
