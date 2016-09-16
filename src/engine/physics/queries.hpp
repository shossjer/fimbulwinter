
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
		void positionOf(const engine::Entity id, Point & pos, Vector & vel, float & angle);
		/**
		 *
		 */
		class RayCast
		{
		public:
			/**
			 *	Callback for RayCast result
			 */
			virtual float callback(const engine::Entity id, const Point & point, const Point & normal, const float fraction) = 0;
			/**
			 *
			 */
			void execute(const Point & from, const Point & to);
		};
	}
}
}

#endif // ENGINE_PHYSICS_QUERIES_HPP
