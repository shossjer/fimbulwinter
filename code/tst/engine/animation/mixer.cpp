#include "config.h"

#include "engine/animation/mixer.hpp"
#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/physics/physics.hpp"
#include "engine/resource/reader.hpp"

#include <catch/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Animation Mixer can be created and destroyed", "[.engine][.animation]")
{
	engine::resource::reader reader;
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandleW(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif
	engine::graphics::renderer renderer(window, reader, nullptr, engine::graphics::renderer::Type::OPENGL_1_2);
	engine::graphics::viewer viewer(renderer);
	engine::physics::simulation simulation(renderer, viewer);

	for (int i = 0; i < 2; i++)
	{
		engine::animation::mixer mixer(renderer, simulation);
	}
}
