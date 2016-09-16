
#ifndef ENGINE_MODEL_ARMATURE_HPP
#define ENGINE_MODEL_ARMATURE_HPP

#include <engine/Entity.hpp>

#include <string>

// the animation system needs to handle the direction of things:
//   "in which direction are we running?"
// this means that the physics system gets the correct position but
// the rendering system does not get the direction:
//   "in which direction does it look like we are running?"

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
