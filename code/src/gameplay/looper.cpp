
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>
#include "engine/replay/reader.hpp"

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
	namespace hid
	{
		namespace ui
		{
			extern void update();
		}
	}
}

namespace
{
	bool active = false;

	long long frame_count;

	core::async::Thread looperThread;
}

namespace gameplay
{
namespace looper
{
	void run();

	void create()
	{
		active = true;

		frame_count = 0;

		looperThread = core::async::Thread{ gameplay::looper::run };
	}
		
	void destroy()
	{
		active = false;

		looperThread.join();
	}

	void run()
	{
		int frame_count = 0;

		engine::replay::start();

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
			::engine::hid::ui::update();
		
			// update characters
			::engine::replay::update(frame_count);
			::gameplay::gamestate::update(frame_count);

			::engine::gui::update();

			//
			::engine::graphics::viewer::update();
			::engine::graphics::renderer::update();

			frame_count++;

			// something temporary that delays
			core::async::delay(15); // ~60 fps
		}
	}
}
}
