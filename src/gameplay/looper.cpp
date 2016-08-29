
#include "input.hpp"

#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>

#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/model/armature.hpp>
#include <engine/physics/Callbacks.hpp>
#include <engine/physics/physics.hpp>

#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

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
	extern void initialize(const engine::physics::Callbacks & callback);
	extern void teardown();
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

	//	looperThread = std::move(looperThread);
	}
		
	void destroy()
	{
		active = false;

		looperThread.join();
	}

	void temp()
	{
		{
			// vvvv tmp vvvv
			const auto modelentity = ::engine::Entity::create();
			::engine::graphics::renderer::add(modelentity,
				::engine::graphics::renderer::asset::CharacterMesh{ "res/player.msh" });
			::engine::model::add(modelentity, ::engine::model::armature{ "res/player.arm" });
			::engine::model::update(modelentity, ::engine::model::action{ "idle-00" });
			// ^^^^ tmp ^^^^
			/**
			 * create Player object
			 */
		//	const auto id = engine::Entity::create();
			::gameplay::characters::create(modelentity);
			::engine::physics::CharacterData capsule{ Point{ { 12.f, 15.f, 0.f } }, ::engine::physics::Material::MEETBAG, .6f, 2.f, .5f }; // 0.6f, 1.8f, 0.4f
			::engine::physics::create(modelentity, capsule);

			gameplay::player::set(modelentity);
		}
		{
			///**
			// * create Boxes
			// */
			//const float S = 0.5f;
			//const float SOLIDITY = 0.06f;
			//const ::engine::physics::Material M = ::engine::physics::Material::WOOD;
			//for (unsigned int i = 2; i < 50; i++)
			//{
			//	const auto id = engine::Entity::create();
			//	{
			//		::engine::physics::BoxData data{ Point{ { 0.f, i*10.f, 0.f } }, M, SOLIDITY, Size{ { S, S, S } } };
			//		::engine::physics::create(id, data);
			//	}
			//	{
			//		engine::graphics::data::CuboidC data = {
			//			core::maths::Matrix4x4f::identity(),
			//			S * 2.f, S * 2.f, S * 2.f,
			//			0xff00ff00
			//		};
			//		engine::graphics::renderer::add(id, data);
			//	}
			//}
		}
	}

	void run()
	{
		class PhysicsCallback : public ::engine::physics::Callbacks
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

		} callbacks;

		::engine::physics::initialize(callbacks);

		// temp
		temp();

		::engine::graphics::renderer::create();

		// 
		while (active)
		{
			// update effects
			::gameplay::effects::update();

			// update animations
			::engine::model::update();

			// update physics
			::engine::physics::update();

			// 
			input::updateInput();
		
			// update characters
			::gameplay::characters::update();

			// get the active Camera
			input::updateCamera();

			//
			::engine::graphics::viewer::update();
			::engine::graphics::renderer::update();

			// something temporary that delays
			core::async::delay(10);
		}

		::engine::graphics::renderer::destroy();
	}
}
}
