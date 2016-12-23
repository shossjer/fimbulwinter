
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
			// vvvv tmp vvvv
			const auto modelentity = ::engine::Entity::create();
			::engine::graphics::renderer::add(modelentity,
				::engine::graphics::renderer::asset::CharacterMesh{ "res/player.msh" });
			::engine::animation::add(modelentity, ::engine::animation::armature{ "res/player.arm" });
			::engine::animation::update(modelentity, ::engine::animation::action{ "stand-00" });
		//	const auto id = engine::Entity::create();
			::gameplay::characters::create(modelentity);
			::engine::physics::CharacterData capsule{ engine::physics::BodyType::CHARACTER, Vector3f{ 12.f, 2.f, 0.f }, ::engine::physics::Material::MEETBAG, .6f, 2.f, .5f }; // 0.6f, 1.8f, 0.4f
			::engine::physics::post_create(modelentity, capsule);

			gameplay::ui::post_add_player(modelentity);

			const auto cameraentity = engine::Entity::create();
			engine::graphics::viewer::add(cameraentity,
			                              engine::graphics::viewer::camera(core::maths::Quaternionf(1.f, 0.f, 0.f, 0.f),
			                                                               Vector3f(0.f, 0.f, 0.f)));
			gameplay::characters::post_add_camera(cameraentity, modelentity);
			// ^^^^ tmp ^^^^
		}
		{
			/**
			 * create Boxes
			 */
			const float S = 0.5f;
			const float SOLIDITY = 0.06f;
			const ::engine::physics::Material M = ::engine::physics::Material::WOOD;
			for (unsigned int i = 0; i < 10; i++)
			{
				const float x = 10.f;
				const float y = i*10.f;
				const float z = 0.f;

				const auto id = engine::Entity::create();
				{
					::engine::physics::BoxData data {::engine::physics::BodyType::DYNAMIC, Vector3f {x, y, z}, M, SOLIDITY, Vector3f {S, S, S}};
					::engine::physics::post_create(id, data);
				}
				{
					engine::graphics::data::CuboidC data = {
						core::maths::Matrix4x4f::identity(),
						S * 2.f, S * 2.f, S * 2.f,
						0xff00ff00
					};
					engine::graphics::renderer::add(id, data);
				}
				{
					engine::graphics::data::ModelviewMatrix data = {
						core::maths::Matrix4x4f::translation(x, y, z) *
						core::maths::Matrix4x4f::rotation(core::maths::radianf {0.f}, 0.f, 0.f, 1.f) *
						core::maths::Matrix4x4f::rotation(core::maths::radianf {0.f}, 0.f, 1.f, 0.f)
					};

					engine::graphics::renderer::update(id, std::move(data));
				}
			}



			for (unsigned int i = 0; i < 10; i++)
			{
				const float w = 2.f;
				const float h = .5f;

				const float x = 0.f + i*(w + 0.2f);	// start x
				const float y = 0.f;		// height
				const float z = 0.f;		// depth

				const auto id = engine::Entity::create();
				{
					::engine::physics::BoxData data {::engine::physics::BodyType::STATIC, Vector3f {x, y, z}, M, SOLIDITY, Vector3f {w*.5f, h*.5f, w*.5f}};
					::engine::physics::post_create(id, data);
				}
				{
					engine::graphics::data::CuboidC data = {
						core::maths::Matrix4x4f::identity(),
						w, h, w,
						0xff00ff00
					};
					engine::graphics::renderer::add(id, data);
				}
				{
					engine::graphics::data::ModelviewMatrix data = {
						core::maths::Matrix4x4f::translation(x, y, z) *
						core::maths::Matrix4x4f::rotation(core::maths::radianf {0.f}, 0.f, 0.f, 1.f) *
						core::maths::Matrix4x4f::rotation(core::maths::radianf {0.f}, 0.f, 1.f, 0.f)
					};

					engine::graphics::renderer::update(id, std::move(data));
				}
			}
		}
	}

	void run()
	{
		class PhysicsCallback : public ::engine::physics::Callback
		{
		public:

			void onGrounded(const engine::Entity id, const core::maths::Vector3f & groundNormal) const
			{
				characters::postGrounded(id, groundNormal);
			}

			void onFalling(const engine::Entity id) const
			{
				characters::postFalling(id);
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
