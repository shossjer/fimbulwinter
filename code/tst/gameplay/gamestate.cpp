#include "config.h"

#include "engine/animation/mixer.hpp"
#include "engine/application/config.hpp"
#include "engine/application/window.hpp"
#include "engine/audio/system.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/hid/ui.hpp"
#include "engine/physics/physics.hpp"
#include "engine/replay/writer.hpp"
#include "engine/resource/reader.hpp"

#include "gameplay/gamestate.hpp"

#include <catch/catch.hpp>

#if WINDOW_USE_USER32
# include <Windows.h>
#endif

TEST_CASE("Gameplay Gamestate can be created and destroyed", "[.engine][.gameplay]")
{
	engine::resource::reader reader;
	engine::record record;
	engine::audio::System audio;
	engine::application::config_t config;
#if WINDOW_USE_USER32
	engine::application::window window(GetModuleHandle(nullptr), SW_HIDE, config);
#else
	engine::application::window window(config);
#endif
	engine::hid::ui ui;
	engine::graphics::renderer renderer(window, reader, nullptr, engine::graphics::renderer::Type::OPENGL_1_2);
	engine::graphics::viewer viewer(renderer);
	engine::physics::simulation simulation(renderer, viewer);
	engine::animation::mixer mixer(renderer, simulation);

	for (int i = 0; i < 2; i++)
	{
		gameplay::gamestate gamestate(mixer, audio, renderer, viewer, ui, simulation, record, reader);
	}
}
