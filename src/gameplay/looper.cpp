
#include "input.hpp"

#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>

#include <engine/graphics/Camera.hpp>
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

	extern void update(const ::engine::graphics::Camera & camera);

	extern void destroy();
}
}
namespace physics
{
	extern void initialize();
	extern void teardown();
	extern void render();
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
	void update(CharacterState & state);
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
			/**
			 * create Player object
			 */
			const engine::Entity id{ 1 };
			::gameplay::characters::create(id);
			::engine::physics::CharacterData capsule{ Point{ { 12.f, 15.f, 0.f } }, ::engine::physics::Material::MEETBAG, 0.6f, 2.f, .5f }; // 0.6f, 1.8f, 0.4f
			::engine::physics::create(id, capsule);

			gameplay::player::set(id);
		}
		{
			/**
			 * create Boxes
			 */
			const float S = 1.0f;
			const float SOLIDITY = 0.1f;
			const ::engine::physics::Material M = ::engine::physics::Material::WOOD;
			for (unsigned int i = 2; i < 50; i++)
			{
				::engine::physics::BoxData data{ Point{ { 0.f, i*10.f, 0.f } }, M, SOLIDITY, Size{ { S, S, S } } };
				::engine::physics::create(i, data);
			}
		}
	}

	void run()
	{
		::engine::physics::initialize();

		// temp
		temp();

		::engine::graphics::renderer::create();

		// 
		while (active)
		{
			// update effects
			::gameplay::effects::update();

			// update fysix
			::engine::physics::update();

			// update player - temp, should be part of generic Character update
			{
				input::updateInput();
				
				CharacterState & character = characters::get(player::get());

				engine::physics::MoveResult res =
					engine::physics::update(1, engine::physics::MoveData(character.movement(), character.fallVel));

				character.grounded = res.grounded;
				character.fallVel = res.velY;
			}

			// update npc's


			// get the active Camera
			const ::engine::graphics::Camera & camera = input::updateCamera();

			// 
			::engine::graphics::renderer::update(camera);

			// something temporary that delays
			core::async::delay(10);
		}

		::engine::graphics::renderer::destroy();
	}

	void update(CharacterState & state)
	{
		engine::physics::MoveResult res = 
			engine::physics::update(0, engine::physics::MoveData(state.movement(), state.fallVel));
	//	engine::physics::update(0, state.movement(), state.fallVel);
		
		state.grounded = res.grounded;
		state.fallVel = res.velY;
	}
}
}
