#include "config.h"

#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/physics/physics.hpp"

#include <catch/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Physics Simulation can be created and destroyed", "[.engine][.physics]")
{
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandleW(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif
	engine::graphics::renderer renderer(window, nullptr, engine::graphics::renderer::Type::OPENGL_1_2);
	engine::graphics::viewer viewer(renderer);

	for (int i = 0; i < 2; i++)
	{
		engine::physics::simulation simulation(renderer, viewer);
	}
}
