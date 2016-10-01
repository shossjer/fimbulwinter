
#ifndef ENGINE_ANIMATION_MIXER_HPP
#define ENGINE_ANIMATION_MIXER_HPP

#include <engine/Entity.hpp>

#include <string>

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
		};

		void add(engine::Entity entity, const armature & data);
		void remove(engine::Entity entity);
		void update(engine::Entity entity, const action & data);
	}
}

#endif /* ENGINE_ANIMATION_MIXER_HPP */
