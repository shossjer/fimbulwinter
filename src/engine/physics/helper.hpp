
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

		// 
		Actor load(const engine::Entity id);
	}

	//std::vector<Id> nearbyAABB(const b2World & world, const b2Vec2 centre, const float halfX, const float halfY)
	////const Point & pos, const float radius, std::vector<Id> & objects)
	//{
	//	std::vector<Id> objects;
	//
	//	// 
	//	class AABBQuery : public b2QueryCallback
	//	{
	//		std::vector<Id> & objects;
	//	public:
	//		AABBQuery(std::vector<Id> & objects) : objects(objects) {}

	//		bool ReportFixture(b2Fixture* fixture)
	//		{
	//			const b2Body *const body = fixture->GetBody();

	//			objects.push_back((Id)body->GetUserData());

	//			// Return true to continue the query.
	//			return true;
	//		}

	//	} query{ objects };

	//	b2AABB aabb{};

	//	aabb.lowerBound.Set(centre.x - halfX, centre.y - halfY);
	//	aabb.upperBound.Set(centre.x + halfX, centre.y + halfY);

	//	world.QueryAABB(&query, aabb);

	//	return std::move(objects);
	//}
	//std::vector<Actor> nearbySphere(const b2World & world, const b2Vec2 centre, const float radius)
	//{
	//	std::vector<Id> objects = nearbyAABB(world, centre, radius, radius);

	//	// TODO: remove objects in "corners"

	//	return std::move(objects);
	//}
	//std::vector<Actor> nearbyCone(const b2World & world, const b2Vec2 start, const b2Vec2 direction)
	//{
	//	// Create AABB values for the Cone
	//	b2Vec2 centre;
	//	float halfX, halfY;

	//	std::vector<Id> objects = nearbyAABB(world, centre, halfX, halfY);

	//	// TODO: filter out objects outside the cone.
	//	std::vector<Id> filtered;

	//	for (auto & object : objects)
	//	{
	//		if (true) filtered.push_back(object);
	//	}

	//	return std::move(filtered);
	//}
}
}

#endif // ENGINE_PHYSICS_HELPER_HPP