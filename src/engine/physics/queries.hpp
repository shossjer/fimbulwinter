
#ifndef ENGINE_PHYSICS_QUERIES_HPP
#define ENGINE_PHYSICS_QUERIES_HPP

#include "defines.hpp"

namespace engine
{
namespace physics
{
	namespace query
	{
		/**
		 *
		 */
		::core::maths::Vector3f positionOf(const engine::Entity id);
		/**
		 *
		 */
		void positionOf(const engine::Entity id, Vector3f & pos, Vector3f & vel, float & angle);
	}
}
}

#endif // ENGINE_PHYSICS_QUERIES_HPP
