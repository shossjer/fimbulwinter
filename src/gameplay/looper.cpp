
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/gamestate.hpp>
#include <gameplay/level.hpp>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			extern void create();

			extern void update();

			extern void destroy();
		}
		namespace viewer
		{
			extern void update();
		}
	}

	namespace physics
	{
		extern void create();

		extern void destroy();
	}
}
namespace gameplay
{
	namespace ui
	{
		extern void update();
	}
}

namespace
{
	bool active = true;

	core::async::Thread looperThread;
}

namespace gameplay
{
namespace looper
{
	void run();

	void create()
	{
		looperThread = core::async::Thread{ gameplay::looper::run };
	}
		
	void destroy()
	{
		active = false;

		looperThread.join();
	}

	void run()
	{
		::engine::physics::create();

		::engine::graphics::renderer::create();

		::gameplay::gamestate::create();

		// temp
		level::create("res/level.lvl");

		// 
		while (active)
		{
			// update actors in engine
			::engine::physics::update_start();

			//
			::engine::physics::update_joints();

			// update physics world
			::engine::physics::update_finish();

			// 
			gameplay::ui::update();
		
			// update characters
			::gameplay::gamestate::update();

			//
			::engine::graphics::viewer::update();
			::engine::graphics::renderer::update();

			// something temporary that delays
			core::async::delay(15); // ~60 fps
		}

		level::destroy();

		::gameplay::gamestate::destroy();

		::engine::graphics::renderer::destroy();

		::engine::physics::destroy();
	}
}
}
