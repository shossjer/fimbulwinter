
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/animation/Callbacks.hpp>
#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/characters.hpp>
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
	namespace animation
	{
		extern void initialize(const engine::animation::Callbacks & callback);
	}

	namespace physics
	{
		extern bool setup();

		extern void teardown();
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

	void add_some_stuff()
	{
		{
			/**
			 *	create limiting planes
			 */
			const auto depth = 1.00f;
			{
				const auto id = engine::Entity::create();
				const Vector3f point{0.f, 0.f, depth};
				const Vector3f normal {0.f, 0.f, -1.f};
				engine::physics::PlaneData data {point, normal, engine::physics::Material::LOW_FRICTION};
				engine::physics::post_create(id, data);
			}
			{
				const auto id = engine::Entity::create();
				const Vector3f point {0.f, 0.f, -depth};
				const Vector3f normal {0.f, 0.f, 1.f};
				engine::physics::PlaneData data {point, normal, engine::physics::Material::LOW_FRICTION};
				engine::physics::post_create(id, data);
			}
		}
		level::create("res/level.lvl");
	}

	void run()
	{
		class AnimationCallback : public ::engine::animation::Callbacks
		{
		public:

			void onFinish(const engine::Entity id) const
			{
				characters::post_animation_finish(id);
			}

		} animationCallbacks;

		::engine::animation::initialize(animationCallbacks);

		::engine::physics::setup();

		::engine::graphics::renderer::create();

		::gameplay::characters::setup();

		// temp
		add_some_stuff();

		// 
		while (active)
		{
			// update animations
			::engine::animation::update();

			// update actors in engine
			::engine::physics::update_start();

			//
			::engine::physics::update_joints();

			// update physics world
			::engine::physics::update_finish();

			// 
			gameplay::ui::update();
		
			// update characters
			::gameplay::characters::update();

			//
			::engine::graphics::viewer::update();
			::engine::graphics::renderer::update();

			// something temporary that delays
			core::async::delay(15); // ~60 fps
		}

		::engine::graphics::renderer::destroy();

		::engine::physics::teardown();
	}
}
}
