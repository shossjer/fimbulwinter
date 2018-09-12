
#ifndef ENGINE_ANIMATION_MIXER_HPP
#define ENGINE_ANIMATION_MIXER_HPP

#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/common.hpp"

#include <string>
#include <vector>

namespace engine
{
	namespace animation
	{
		void update();

		struct armature
		{
			std::string armfile;
		};

		struct action
		{
			std::string name;
			bool repetative;
		};

		void add(engine::Entity entity, const armature & data);
		void remove(engine::Entity entity);
		void update(engine::Entity entity, const action & data);

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

		void add(engine::Asset asset, object && data);
		void add_model(engine::Entity entity, engine::Asset asset);
		void remove(engine::Asset asset);
	}
}

#endif /* ENGINE_ANIMATION_MIXER_HPP */
