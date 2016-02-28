
#ifndef ENGINE_PHYSICS_COLLISION_HPP
#define ENGINE_PHYSICS_COLLISION_HPP

#include <core/maths/Vector.hpp>

namespace engine
{
	namespace physics
	{
		struct Body
		{
			double mass;
			double mofi;

			struct
			{
				core::maths::Vector2d position;
				core::maths::Vector2d velocity;
				core::maths::Vector2d force;
				double                rotation;
				double                angularv;
				double                torque;
			} buffer[2];
		};

		struct Plane
		{
			Body body;

			core::maths::Vector2d normal;
			double d;
		};
		struct Rectangle
		{
			Body body;

			double width;
			double height;
		};
	}
}

#endif /* ENGINE_PHYSICS_COLLISION_HPP */
