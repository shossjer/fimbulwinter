
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/animation/Callbacks.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>
#include <gameplay/level.hpp>
#include <gameplay/ui.hpp>

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

		extern void subscribe(const engine::physics::Callback & callback);
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
	using ActorData = engine::physics::ActorData;
	using ShapeData = engine::physics::ShapeData;
	using Material = engine::physics::Material;

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

	engine::Entity playerId {engine::Entity::INVALID};

	void physics_box(const engine::Entity id, const ActorData::Type type, const ActorData::Behaviour behaviour, const Material material, const float solidity, const float x, const float y, const float z, const float w, const float h, const float d)
	{
		std::vector<ShapeData> shapes;
		shapes.push_back(ShapeData {
			ShapeData::Type::BOX,
			material,
			solidity,
			Vector3f{0.f, 0.f, 0.f},
			Quaternionf{1.f, 0.f, 0.f, 0.f},
			ShapeData::Geometry{ShapeData::Geometry::Box{w*0.5f, h*0.5f, d*0.5f} } });

		ActorData data {type, behaviour, x, y, z, shapes};

		::engine::physics::post_create(id, data);
	}

	void renderer_box(const engine::Entity id, const engine::graphics::data::Color color, const float x, const float y, const float z, const float w, const float h, const float d)
	{
		engine::graphics::data::CuboidC data = {
			core::maths::Matrix4x4f::translation(x, y, z), w, h, d, color
		};
		engine::graphics::renderer::add(id, data);
	}

	void add_some_stuff()
	{
		{
			playerId = engine::Entity::create();
			::gameplay::characters::create(playerId);

			{
				const float w = 1.2f;
				const float h = 0.8f;
				const float d = 2.f;

				const float x = 14.f;
				const float y = 3.f;
				const float z = 0.f;

				physics_box(playerId, ActorData::Type::DYNAMIC, ActorData::Behaviour::PLAYER, Material::OILY_ROBOT, .75f, x, y, z, w, h, d);
				renderer_box(playerId, 0xffff00ff, x, y, z, w, h, d);
			}

			gameplay::ui::post_add_player(playerId);

			{
				const auto cameraId = engine::Entity::create();
				engine::graphics::viewer::add(cameraId,
					engine::graphics::viewer::camera(core::maths::Quaternionf(1.f, 0.f, 0.f, 0.f),
						Vector3f(0.f, 0.f, 0.f)));
				gameplay::characters::post_add_camera(cameraId, playerId);
			}
		}
		{
			/**
			 *	create limiting planes
			 */
			const auto depth = 1.00f;
			{
				const auto id = engine::Entity::create();
				const Vector3f point{0.f, 0.f, depth};
				const Vector3f normal {0.f, 0.f, -1.f};
				::engine::physics::PlaneData data {point, normal, engine::physics::Material::LOW_FRICTION};
				engine::physics::post_create(id, data);
			}
			{
				const auto id = engine::Entity::create();
				const Vector3f point {0.f, 0.f, -depth};
				const Vector3f normal {0.f, 0.f, 1.f};
				::engine::physics::PlaneData data {point, normal, engine::physics::Material::LOW_FRICTION};
				engine::physics::post_create(id, data);
			}
		}
		level::create("res/level.lvl");
	}

	void temp_update()
	{
	}

	void run()
	{
		class PhysicsCallback : public ::engine::physics::Callback
		{
		public:

			void postContactFound(const ::engine::Entity ids[2], const ::engine::physics::Material materials[2]) const override
			{
				// TODO: add on queue
			}

			void postContactLost(const ::engine::Entity ids[2]) const override
			{
				// TODO: add on queue
			}
		} physicsCallback;

		::engine::physics::subscribe(physicsCallback);

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

		// temp
		add_some_stuff();

		// 
		while (active)
		{
			// update effects
			::gameplay::effects::update();

			// update animations
			::engine::animation::update();

			temp_update();

			// update actors in engine
			::engine::physics::update_start();

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
			core::async::delay(10);
		}

		::engine::graphics::renderer::destroy();

		::engine::physics::teardown();
	}
}
}
