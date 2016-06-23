
#include "player.hpp"

#include <gameplay/CharacterState.hpp>

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
	extern void create(const ::engine::graphics::Camera & camera);

	extern void update();

	extern void destroy2();
}
}
namespace physics
{
	extern void initialize();
	extern void teardown();
//	extern MoveResult update(const std::size_t object, const std::array<float, 2> vXZ, const float vY);
	extern void render();
}
}

namespace gameplay
{
namespace input
{
	extern void setup();
	extern void setup(CharacterState * player);
//	extern void update();
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

	void run()
	{
		::engine::physics::initialize();

		{	// temp
			{
				::engine::physics::CharacterData capsule{ ::engine::physics::Point{ 10.f, 10.f, 0.f }, ::engine::physics::Material::MEETBAG, 0.6f, 1.8f, 0.4f };
				::engine::physics::create(1, capsule);
			}
			const float S = 1.0f;
			const float SOLIDITY = 0.1f;
			const ::engine::physics::Material M = ::engine::physics::Material::WOOD;
			{
				::engine::physics::BoxData data{ ::engine::physics::Point{ 0.f, 10.f, 0.f }, M, SOLIDITY, ::engine::physics::Size{ S, S, S } };
				::engine::physics::create(2, data);
			}
			{
				::engine::physics::BoxData data{ ::engine::physics::Point{ 0.f, 20.f, 0.f }, ::engine::physics::Material::STONE, 0.5f, ::engine::physics::Size{ S, S, S } };
				create(3, data);
			}
			{
				::engine::physics::BoxData data{ ::engine::physics::Point{ 0.f, 30.f, 0.f }, M, SOLIDITY, ::engine::physics::Size{ S, S, S } };
				::engine::physics::create(4, data);
			}
			{
				::engine::physics::BoxData data{ ::engine::physics::Point{ 0.f, 40.f, 0.f }, M, SOLIDITY, ::engine::physics::Size{ S, S, S } };
				::engine::physics::create(5, data);
			}
		}

		::engine::graphics::Camera camera;

		::engine::graphics::renderer::create(camera);
		/**
		 * create Player object
		 */
		CharacterState playerState;

		input::setup(&playerState);

		// 
		while (active)
		{
			// update fysix
			::engine::physics::update();

			// update player
			{
			//	input::update();

				
				engine::physics::MoveResult res =
					engine::physics::update(1, engine::physics::MoveData(playerState.movement(), playerState.fallVel));

				playerState.grounded = res.grounded;
				playerState.fallVel = res.velY;

				camera.position(res.pos[0], res.pos[1], res.pos[2]);
			}

			// update npc's

			// 
			::engine::graphics::renderer::update();

			// something temporary that delays
			core::async::delay(10);
		}

		input::setup();

		::engine::graphics::renderer::destroy2();
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
