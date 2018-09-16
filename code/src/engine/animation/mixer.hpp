
#ifndef ENGINE_ANIMATION_MIXER_HPP
#define ENGINE_ANIMATION_MIXER_HPP

#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/common.hpp"
#include "engine/animation/Armature.hpp"

#include <string>
#include <vector>

namespace engine
{
	namespace animation
	{
		void update();

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

				bool operator == (const std::string & name) const
				{
					return this->name == name;
				}
			};

			std::string name; // debug purpose
			std::vector<action> actions;
		};

		struct character
		{
			engine::Asset armature;
		};

		struct model
		{
			engine::Asset object;
		};

		struct action
		{
			std::string name;
			bool repetative;
		};

		void post_register_armature(engine::Asset asset, engine::animation::Armature && data);
		void post_register_object(engine::Asset asset, object && data);

		void post_add_character(engine::Entity entity, character && data);
		void post_add_model(engine::Entity entity, model && data);

		void post_update_action(engine::Entity entity, action && data);

		void post_remove(engine::Entity entity);
	}
}

#endif /* ENGINE_ANIMATION_MIXER_HPP */
