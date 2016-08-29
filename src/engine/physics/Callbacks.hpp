 
#ifndef ENGINE_PHYSICS_CALLBACKS_HPP
#define ENGINE_PHYSICS_CALLBACKS_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>

namespace engine
{
namespace physics
{
	class Callbacks
	{
	public:
		/**
		 *
		 */
		virtual void onGrounded(const engine::Entity id, const core::maths::Vector3f & groundNormal) const = 0;
		/**
		 *
		 */
		virtual void onFalling(const engine::Entity id) const = 0;
	};
}
}

#endif ENGINE_PHYSICS_CALLBACKS_HPP
