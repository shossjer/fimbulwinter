
#include "defines.hpp"

#include <Box2D/Box2D.h>

namespace engine
{
namespace physics
{
	namespace query
	{
		struct Actor
		{
			const engine::Entity id;
			b2Body *const body;

			Actor(const engine::Entity id, b2Body *const body)
				:
				id{ id },
				body{ body }
			{}
		};

		// define queries
		std::vector<Actor> nearby(const b2Vec2 centre, const float halfX, const float halfY);

		// 
		std::vector<Actor> load(const std::vector<engine::Entity> & targets);

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
