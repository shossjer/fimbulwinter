#include "looper.hpp"

#include "core/async/delay.hpp"
#include "core/async/Thread.hpp"

#include "engine/physics/Callback.hpp"
#include "engine/replay/reader.hpp"

#include "gameplay/gamestate.hpp"

namespace engine
{
	namespace animation
	{
		extern void update(mixer & mixer);
	}

	namespace graphics
	{
		extern void update(renderer & renderer);
		extern void update(viewer & viewer);
	}

	namespace hid
	{
		extern void update(ui & ui);
	}

	namespace physics
	{
		extern void update_start(simulation & simulation);
		extern void update_joints(simulation & simulation);
		extern void update_finish(simulation & simulation);
	}
}

namespace gameplay
{
	extern void update(gamestate & gamestate, int frame_count);
}

namespace
{
	engine::animation::mixer * mixer = nullptr;
	engine::graphics::renderer * renderer = nullptr;
	engine::graphics::viewer * viewer = nullptr;
	engine::hid::ui * ui = nullptr;
	engine::physics::simulation * simulation = nullptr;
	gameplay::gamestate * gamestate = nullptr;

	bool active = false;

	core::async::Thread looperThread;

	void run()
	{
		int frame_count = 0;

		engine::replay::start(gameplay::post_command);

		while (active)
		{
			update(*::mixer);

			// update actors in engine
			update_start(*::simulation);

			//
			update_joints(*::simulation);

			// update physics world
			update_finish(*::simulation);

			// 
			update(*::ui);
		
			// update characters
			::engine::replay::update(frame_count);
			update(*::gamestate, frame_count);

			//
			update(*::viewer);
			update(*::renderer);

			frame_count++;

			// something temporary that delays
			core::async::delay(15); // ~60 fps
		}
	}
}

namespace gameplay
{
	looper::~looper()
	{
		active = false;

		looperThread.join();

		::gamestate = nullptr;
		::simulation = nullptr;
		::ui = nullptr;
		::viewer = nullptr;
		::renderer = nullptr;
		::mixer = nullptr;
	}

	looper::looper(engine::animation::mixer & mixer_, engine::graphics::renderer & renderer_, engine::graphics::viewer & viewer_, engine::hid::ui & ui_, engine::physics::simulation & simulation_, gameplay::gamestate & gamestate_)
	{
		::mixer = &mixer_;
		::renderer = &renderer_;
		::viewer = &viewer_;
		::ui = &ui_;
		::simulation = &simulation_;
		::gamestate = &gamestate_;

		active = true;

		looperThread = core::async::Thread(run);
	}
}
