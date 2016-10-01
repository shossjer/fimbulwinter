
#include <core/async/delay.hpp>
#include <core/async/Thread.hpp>

#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/physics/Callbacks.hpp>
// #include <engine/physics/effects.hpp>
#include <engine/physics/physics.hpp>
#include <engine/physics/queries.hpp>

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
	namespace physics
	{
		extern void initialize(const engine::physics::Callbacks & callback);
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
			::engine::animation::add(modelentity, ::engine::animation::armature{ "res/player.arm" });
			::engine::animation::update(modelentity, ::engine::animation::action{ "stand-00" });
		//	const auto id = engine::Entity::create();
			::gameplay::characters::create(modelentity);
			::engine::physics::CharacterData capsule{ Point{ { 12.f, 15.f, 0.f } }, ::engine::physics::Material::MEETBAG, .6f, 2.f, .5f }; // 0.6f, 1.8f, 0.4f
			::engine::physics::create(modelentity, capsule);

			gameplay::ui::post_add_player(modelentity);

			const auto cameraentity = engine::Entity::create();
			engine::graphics::viewer::add(cameraentity,
			                              engine::graphics::viewer::camera(core::maths::Quaternionf(1.f, 0.f, 0.f, 0.f),
			                                                               core::maths::Vector3f(0.f, 0.f, 0.f)));
			gameplay::characters::post_add_camera(cameraentity, modelentity);
			// ^^^^ tmp ^^^^
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
			::engine::animation::update();

			// update physics
			::engine::physics::update();

			// 
			gameplay::ui::update();
		
			// update characters
			::gameplay::characters::update();

			// update player - temp, should be part of generic Character update
			// {
			// 	input::updateInput();
				
			// 	CharacterState & character = characters::get(player::get());

			// 	debug_printline(0xffffffff, "(", character.movement()[0], ", ", character.movement()[1], ")");
			// 	engine::physics::MoveResult res =
			// 		engine::physics::update(gameplay::player::get(), engine::physics::MoveData(character.movement(), character.fallVel, character.angvel));

			// 	character.grounded = res.grounded;
			// 	character.fallVel = res.velY;

			// 	Point pos;
			// 	Vector vec;
			// 	float angle;
			// 	engine::physics::query::positionOf(player::get(), pos, vec, angle);

			// 	engine::physics::effect::acceleration(core::maths::Vector3f{0.f + 9.82f * std::cos(angle - 3.14f / 2.f), 9.82f + 9.82f * std::sin(angle - 3.14f / 2.f), 0.f}, core::maths::Vector3f{pos[0], pos[1], 0.f}, 2.f);
			// }

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
