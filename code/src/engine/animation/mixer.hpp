#pragma once

#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include "engine/animation/Armature.hpp"
#include "engine/common.hpp"
#include "engine/Token.hpp"

#include <string>
#include <vector>

namespace engine
{
	namespace graphics
	{
		class renderer;
	}

	namespace physics
	{
		class simulation;
	}
}

namespace engine
{
	namespace animation
	{
		class mixer
		{
		public:
			~mixer();
			mixer(engine::graphics::renderer & renderer, engine::physics::simulation & simulation);
		};

		void update(mixer & mixer);

		struct object
		{
			struct action
			{
				struct key
				{
					core::maths::Vector3f translation;
					core::maths::Quaternionf rotation;
				};

				std::string name;
				std::vector<key> keys;
			};

			std::string name; // debug purpose
			std::vector<action> actions;
		};

		struct character
		{
			engine::Token armature;
		};

		struct model
		{
			engine::Token object;
		};

		struct action
		{
			std::string name;
			bool repetative;
		};

		void post_register_armature(mixer & mixer, engine::Token asset, engine::animation::Armature && data);
		void post_register_object(mixer & mixer, engine::Token asset, object && data);

		void post_add_character(mixer & mixer, engine::Token entity, character && data);
		void post_add_model(mixer & mixer, engine::Token entity, model && data);

		void post_update_action(mixer & mixer, engine::Token entity, action && data);

		void post_remove(mixer & mixer, engine::Token entity);
	}
}
