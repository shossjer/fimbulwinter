#include "config.h"

#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"

#include <catch2/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Graphics Viewer can be created and destroyed", "[.engine][.graphics]")
{
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandleW(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif
	engine::graphics::renderer renderer(window, nullptr, engine::graphics::renderer::Type::DUMMY_HACK);

	for (int i = 0; i < 2; i++)
	{
		engine::graphics::viewer viewer(renderer);
	}
}
