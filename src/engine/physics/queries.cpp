
#include <config.h>

#include "defines.hpp"
#include "queries.hpp"

#include <Box2D/Box2D.h>

#include <core/maths/Vector.hpp>

#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Font.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include <array>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <utility>

namespace engine
{
namespace physics
{
	namespace
	{
		b2Vec2 convert(const Point & point) { return b2Vec2{ point[0], point[1] }; }
		Point convert(const b2Vec2 & point) { return Point{ point.x, point.y, 0.f }; }
	}

	extern const b2World & getWorld();

	namespace query
	{
		std::vector<Actor> nearby(const b2Vec2 centre, const float halfX, const float halfY)
		{
			std::vector<Actor> objects;

			// 
			class AABBQuery : public b2QueryCallback
			{
				std::vector<Actor> & objects;
			public:
				AABBQuery(std::vector<Actor> & objects) : objects(objects) {}

				bool ReportFixture(b2Fixture* fixture)
				{
					b2Body *const body = fixture->GetBody();

					const engine::Entity id = engine::Entity{ reinterpret_cast<engine::Entity::value_type>(body->GetUserData()) };

					objects.push_back(Actor{ id, body });

					// Return true to continue the query.
					return true;
				}

			} query{ objects };

			b2AABB aabb{};

			aabb.lowerBound.Set(centre.x - halfX, centre.y - halfY);
			aabb.upperBound.Set(centre.x + halfX, centre.y + halfY);

			getWorld().QueryAABB(&query, aabb);

			return objects;
		}

		void RayCast::execute(const Point & from, const Point & to)
		{
			// This class captures the closest hit shape.
			class MyRayCastCallback : public b2RayCastCallback
			{
				RayCast & reporter;
			public:
				MyRayCastCallback(RayCast & reporter) : reporter(reporter) {}

				float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
				{
					return reporter.callback(
						engine::Entity{ reinterpret_cast<engine::Entity::value_type>(fixture->GetBody()->GetUserData()) }, 
						convert(point),
						convert(normal), 
						fraction);
				}
			} query{ *this };

			getWorld().RayCast(&query, convert(from), convert(to));
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
}
