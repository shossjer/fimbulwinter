#include <catch.hpp>

#include "config.h"

#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/resource/reader.hpp"

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Graphics Renderer can be created and destroyed", "[.engine][.graphics]")
{
	engine::resource::reader reader;
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandle(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif

	for (int i = 0; i < 2; i++)
	{
		engine::graphics::renderer renderer(window, reader, nullptr, engine::graphics::renderer::Type::OPENGL_1_2);
	}

	for (int i = 0; i < 2; i++)
	{
		engine::graphics::renderer renderer(window, reader, nullptr, engine::graphics::renderer::Type::OPENGL_3_0);
	}
}
