
#include <config.h>

#include "effects.hpp"

#include "helper.hpp"

namespace engine
{
namespace physics
{
namespace effect
{
	/**
	 *	Apply acceleration on every object inside radius
	 *	Returns list of targets
	 */
	std::vector<engine::Entity> acceleration(const Vector amount, const Point centre, const float radius)
	{
		const b2Vec2 acc{ amount[0], amount[1] };

		// find nearby Actors
		std::vector<query::Actor> actors = query::nearby(b2Vec2{ centre[0], centre[1] }, radius, radius);

		std::vector<engine::Entity> ids;
		ids.reserve(actors.size());

		// apply acceleration
		for (const auto item : actors)
		{
			ids.push_back(item.id);

			applyAcceleration(item.body, acc);
		}

		return ids;
	}
	/**
	 *	Apply acceleration on targets from prev query
	 */
	void acceleration(const Vector amount, const std::vector<engine::Entity> & targets)
	{
		const b2Vec2 acc{ amount[0], amount[1] };

		// bind target Actor
		std::vector<query::Actor> actors = query::load(targets);

		// apply acceleration
		for (const auto item : actors)
		{
			applyAcceleration(item.body, acc);
		}
	}
	///**
	// *	Apply force on every object inside radius
	// *	Returns list of targets
	// */
	//std::vector<Id> & force(const Vector amount, const Point centre, const float radius)
	//{

	//}
	///**
	// *	Apply acceleration on targets from prev query
	// */
	//void force(const Vector amount, const std::vector<Id>& targets)
	//{

	//}
}
}
}
