#pragma once

namespace engine
{
	namespace animation
	{
		class mixer;
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
}

namespace gameplay
{
	class gamestate;
}

namespace gameplay
{
	class looper
	{
	public:
		~looper();
		looper(engine::animation::mixer & mixer, engine::graphics::renderer & renderer, engine::graphics::viewer & viewer, engine::hid::ui & ui, engine::physics::simulation & simulation, gameplay::gamestate & gamestate);
	};
}
