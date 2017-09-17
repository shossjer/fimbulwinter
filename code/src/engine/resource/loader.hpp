
#ifndef ENGINE_EXTERNAL_LOADER_HPP
#define ENGINE_EXTERNAL_LOADER_HPP

#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>

#include <string>

namespace engine
{
	namespace external
	{
		namespace loader
		{
			struct Level
			{
				std::string name;
			};
			struct Placeholder
			{
				std::string name;
				core::maths::Vector3f translation;
				core::maths::Quaternionf rotation;
				core::maths::Vector3f scale;
			};
		}

		void post_load_level(engine::Entity entity, loader::Level && data);
		void post_load_placeholder(engine::Entity entity, loader::Placeholder && data);
	}
}

#endif /* ENGINE_EXTERNAL_LOADER_HPP */
