 
#ifndef ENGINE_ANIMATION_CALLBACKS_HPP
#define ENGINE_ANIMATION_CALLBACKS_HPP

#include <engine/Entity.hpp>

namespace engine
{
namespace animation
{
	class Callbacks
	{
	public:
		/**
		 *
		 */
		virtual void onFinish(const engine::Entity id) const = 0;
	};
}
}

#endif /* ENGINE_ANIMATION_CALLBACKS_HPP */
