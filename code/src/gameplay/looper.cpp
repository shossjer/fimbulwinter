
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/gamestate.hpp>

namespace engine
{
namespace animation
{
	extern void update();
}
namespace graphics
{
namespace renderer
{
	extern void update();
}
namespace viewer
{
	extern void update();
}
}
namespace gui
{
	extern void update();
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
		// 
		while (active)
		{
			::engine::animation::update();

			// update actors in engine
			::engine::physics::update_start();

			//
			::engine::physics::update_joints();

			// update physics world
			::engine::physics::update_finish();

			// 
			::gameplay::ui::update();
		
			// update characters
			::gameplay::gamestate::update();

			::engine::gui::update();

			//
			::engine::graphics::viewer::update();
			::engine::graphics::renderer::update();

			// something temporary that delays
			core::async::delay(15); // ~60 fps
		}
	}
}
}
