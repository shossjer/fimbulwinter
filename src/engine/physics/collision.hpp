
#ifndef ENGINE_PHYSICS_COLLISION_HPP
#define ENGINE_PHYSICS_COLLISION_HPP

#include <core/maths/Plane.hpp>
#include <core/maths/Scalar.hpp>
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
				core::maths::Scalard  rotation;
				core::maths::Scalard  angularv;
				core::maths::Scalard  torque;
			} buffer[2];
		};

		struct Circle
		{
			Body body;

			double radie;

			void calc_body(const double density) 
			{
				this->body.mass = 3.1415926f*this->radie*density;
				this->body.mofi = 0.5*this->body.mass*this->radie*this->radie;
			}
		};
		struct Plane
		{
			Body body;

			core::maths::Plane2d plane;
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
