
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
		extern void setup();

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
	engine::Entity platformId {engine::Entity::INVALID};
	engine::Entity spinnerId {engine::Entity::INVALID};

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
		engine::graphics::data::CuboidC dataShape = {
			core::maths::Matrix4x4f::identity(), w, h, d, color
		};
		engine::graphics::renderer::add(id, dataShape);

		engine::graphics::data::ModelviewMatrix dataMatrix = {
			core::maths::Matrix4x4f::translation(x, y, z)
		};
		engine::graphics::renderer::update(id, std::move(dataMatrix));
	}

	void add_some_stuff()
	{
		{
			playerId = engine::Entity::create();
			::gameplay::characters::create(playerId);

			{
				const float w = 1.2f;
				const float h = 0.8f;
				const float d = 1.f;

				const float x = 14.f;
				const float y = 3.f;
				const float z = 0.f;

				physics_box(playerId, ActorData::Type::DYNAMIC, ActorData::Behaviour::PLAYER, Material::OILY_ROBOT, .75f, x, y, z, w, h, d);
				renderer_box(playerId, 0x00ff00ff, x, y, z, w, h, d);
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
			const auto depth = 0.50f;
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
		{
			// add an Kinematic object for platform
			const float w = 2.f;
			const float h = 0.5f;
			const float d = 1.f;

			const float x = 13.f;
			const float y = 4.f;
			const float z = 0.f;

			platformId = engine::Entity::create();
			{
				physics_box(platformId, ActorData::Type::KINEMATIC, ActorData::Behaviour::OBSTACLE, Material::STONE, 1.f, x, y, z, w, h, d);
				renderer_box(platformId, 0x0000ff00, x, y, z, w, h, d);
			}
		}
		{
			// add an Kinematic object for spinner
			const float w = 7.f;
			const float h = 0.2f;
			const float d = 1.f;

			const float x = 8.f;
			const float y = 4.f;
			const float z = 0.f;

			spinnerId = engine::Entity::create();
			{
				physics_box(spinnerId, ActorData::Type::KINEMATIC, ActorData::Behaviour::OBSTACLE, Material::STONE, 1.f, x, y, z, w, h, d);
				renderer_box(spinnerId, 0x0000ff00, x, y, z, w, h, d);
			}
		}

		/**
		 * create Boxes
		 */
		for (unsigned int i = 0; i < 20; i++)
		{
			const float w = 0.8f;
			const float h = 0.8f;
			const float d = 1.f;

			const float x = 10.f;
			const float y = i*2.f + 1.f;
			const float z = 0.f;

			const float SOLIDITY = 0.06f;

			const auto id = engine::Entity::create();
			{
				physics_box(id, ActorData::Type::DYNAMIC, ActorData::Behaviour::DEFAULT, Material::WOOD, SOLIDITY, x, y, z, w, h, d);
				renderer_box(id, 0xff00ffff, x, y, z, w, h, d);
			}
		}

		for (unsigned int i = 0; i < 10; i++)
		{
			const float w = 2.f;
			const float h = .5f;
			const float d = 1.f;

			const float x = 0.f + i*(w + 0.2f);	// start x
			const float y = 0.f;		// height
			const float z = 0.f;		// depth

			const auto id = engine::Entity::create();
			{
				physics_box(id, ActorData::Type::STATIC, ActorData::Behaviour::DEFAULT, Material::WOOD, 1.f, x, y, z, w, h, d);
				renderer_box(id, 0xffffff00, x, y, z, w, h, d);
			}
		}
	}

	void temp_update()
	{
		if (platformId!= engine::Entity::INVALID)
		{
			// update moving platform
			static int count = 0;
			static float direction = 1.f;
			static Vector3f position {13.f, 4.f, 0.f};

			engine::physics::post_update_movement(platformId, engine::physics::translation_data {position, Quaternionf{1.f, 0.f, 0.f, 0.f}});

			if (count++>3*50) count = 0, direction = -direction;

			position += Vector3f {direction, 0.f, 0.f}*(1/50.f);
		}

		if (spinnerId!=engine::Entity::INVALID)
		{
			// update rotating beam
			static float angle = 0.f;

			angle += 0.5f*(1/50.f);

			const float w = cos(angle * 0.5f);
			const float x = 0.f;
			const float y = 0.f;
			const float z = sin(angle * 0.5f);

			Vector3f p {8.f, 4.f, 0.f};
			core::maths::Quaternionf q {w, x, y, z};

			engine::physics::post_update_movement(spinnerId, engine::physics::translation_data {p, q});
		}
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
