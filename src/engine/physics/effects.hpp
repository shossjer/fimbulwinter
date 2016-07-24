
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
	std::vector<engine::Entity> acceleration(const Vector amount, const Point centre, const float radius);
	/**
	 *	Apply acceleration on targets from prev query
	 */
	void acceleration(const Vector amount, const std::vector<engine::Entity> & targets);
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