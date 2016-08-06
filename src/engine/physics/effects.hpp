
#include "defines.hpp"

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
	std::vector<engine::Entity> acceleration(const core::maths::Vector3f amount, const core::maths::Vector3f centre, const float radius);
	/**
	 *	Apply acceleration on targets from prev query
	 */
	void acceleration(const core::maths::Vector3f amount, const std::vector<engine::Entity> & targets);


	/**
	 *	Apply force on every object inside radius
	 *	Returns list of targets
	 */
	std::vector<engine::Entity> force(const core::maths::Vector3f amount, const core::maths::Vector3f centre, const float radius);
	/**
	 *	Apply force on targets from prev query
	 */
	void force(const core::maths::Vector3f amount, const std::vector<engine::Entity> & targets);
}
}
}