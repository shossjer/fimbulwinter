
#ifndef ENGINE_MODEL_ARMATURE_HPP
#define ENGINE_MODEL_ARMATURE_HPP

#include <engine/Entity.hpp>

#include <string>

namespace engine
{
	namespace model
	{
		void update();

		struct armature
		{
			std::string armfile;
		};

		struct action
		{
			std::string name;
		};

		void add(engine::Entity entity, const armature & data);
		void remove(engine::Entity entity);
		void update(engine::Entity entity, const action & data);
	}
}

#endif /* ENGINE_MODEL_ARMATURE_HPP */
