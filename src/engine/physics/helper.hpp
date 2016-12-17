
#ifndef ENGINE_PHYSICS_HELPER_HPP
#define ENGINE_PHYSICS_HELPER_HPP

#include <config.h>

#include "defines.hpp"

#include <Box2D/Box2D.h>

#include <core/debug.hpp>

namespace engine
{
namespace physics
{
	namespace
	{
		/**
		 *	Temp function to solve strange issue with Box2D
		 *  Forces are applied for each fixture in the body
		 *	so the force needs to be divided with num items
		 */
		unsigned int countLinkedList(const b2Fixture * fixture)
		{
			unsigned int num = 1;

			//	debug_assert(fixture != nullptr);

			while ((fixture = fixture->GetNext()) != nullptr)
			{
				num++;
			}

			return num;
		}
	}

	inline void applyForce(b2Body *const body, const b2Vec2 force)
	{
		body->ApplyForceToCenter(force, true);
	}
	inline void applyForce(b2Body *const body, const b2Vec2 force, const b2Vec2 atPosiiton)
	{
		body->ApplyForce(force, atPosiiton, true);
	}

	inline void applyAcceleration(b2Body *const body, b2Vec2 acceleration)
	{
		// count number of fixtures to solve strange issue in box2D
		const unsigned int num = countLinkedList(body->GetFixtureList());

		b2Vec2 force = acceleration;
		force *= (body->GetMass() / num);

		body->ApplyForceToCenter(force, true);
	}
	inline void applyAcceleration(b2Body *const body, const b2Vec2 acceleration, const b2Vec2 atPosiiton)
	{
		// count number of fixtures to solve strange issue in box2D
		const unsigned int num = countLinkedList(body->GetFixtureList());

		b2Vec2 force = acceleration;
		force *= (body->GetMass() / num);

		body->ApplyForce(force, atPosiiton, true);
	}
}
}

#endif // ENGINE_PHYSICS_HELPER_HPP
