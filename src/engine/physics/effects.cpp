
#include <config.h>

#include "effects.hpp"

#include "helper.hpp"

namespace engine
{
namespace physics
{
namespace effect
{
	b2Vec2 convert(const core::maths::Vector3f val)
	{
		core::maths::Vector3f::array_type buffer;
		val.get_aligned(buffer);
		return b2Vec2{ buffer[0], buffer[1] };
	}

	/**
	 *	Apply acceleration on every object inside radius
	 *	Returns list of targets
	 */
	std::vector<engine::Entity> acceleration(const core::maths::Vector3f amount, const core::maths::Vector3f centre, const float radius)
	{
		const b2Vec2 acc = convert(amount);

		// find nearby Actors
		std::vector<query::Actor> actors = query::nearby(convert(centre), radius, radius);

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
	void acceleration(const core::maths::Vector3f amount, const std::vector<engine::Entity> & targets)
	{
		const b2Vec2 acc = convert(amount);

		// bind target Actor
		std::vector<query::Actor> actors = query::load(targets);

		// apply acceleration
		for (const auto item : actors)
		{
			applyAcceleration(item.body, acc);
		}
	}
	

	/**
	 *	Apply force on every object inside radius
	 *	Returns list of targets
	 */
	std::vector<engine::Entity> force(const core::maths::Vector3f amount, const core::maths::Vector3f centre, const float radius)
	{
		const b2Vec2 acc = convert(amount);

		// find nearby Actors
		std::vector<query::Actor> actors = query::nearby(convert(centre), radius, radius);

		std::vector<engine::Entity> ids;
		ids.reserve(actors.size());

		// apply acceleration
		for (const auto item : actors)
		{
			ids.push_back(item.id);

			applyForce(item.body, acc);
		}

		return ids;
	}
	/**
	 *	Apply force on targets from prev query
	 */
	void force(const core::maths::Vector3f amount, const std::vector<engine::Entity> & targets)
	{
		const b2Vec2 acc = convert(amount);

		// bind target Actor
		std::vector<query::Actor> actors = query::load(targets);

		// apply acceleration
		for (const auto item : actors)
		{
			applyForce(item.body, acc);
		}
	}
}
}
}
