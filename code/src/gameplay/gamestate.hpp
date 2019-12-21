
#ifndef GAMEPLAY_GAMESTATE_HPP
#define GAMEPLAY_GAMESTATE_HPP

#include "engine/Command.hpp"
#include "engine/Entity.hpp"
#include "engine/common.hpp"

#include "utility/any.hpp"

namespace engine
{
	namespace animation
	{
		class mixer;
	}

	namespace audio
	{
		class System;
	}

	namespace graphics
	{
		class renderer;
		class viewer;
	}

	namespace hid
	{
		class ui;
	}

	namespace physics
	{
		class simulation;
	}

	class record;

	namespace resource
	{
		class reader;
	}
}

namespace gameplay
{
	class gamestate
	{
	public:
		enum class WorkstationType
		{
			BENCH,
			OVEN
		};

	public:
		~gamestate();
		gamestate(engine::animation::mixer & mixer, engine::audio::System & audio, engine::graphics::renderer & renderer, engine::graphics::viewer & viewer, engine::hid::ui & ui, engine::physics::simulation & simulation, engine::record & record, engine::resource::reader & reader);
	};

	void post_command(gamestate & gamestate, engine::Entity entity, engine::Command command);
	void post_command(gamestate & gamestate, engine::Entity entity, engine::Command command, utility::any && data);

	void post_add_workstation(
		gamestate & gamestate,
		engine::Entity entity,
		gamestate::WorkstationType type,
		core::maths::Matrix4x4f front,
		core::maths::Matrix4x4f top);

	void post_add_worker(gamestate & gamestate, engine::Entity entity);
}

#endif /* GAMEPLAY_GAMESTATE_HPP */
