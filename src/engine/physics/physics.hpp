
#ifndef ENGINE_PHYSICS_PHYSICS_HPP
#define ENGINE_PHYSICS_PHYSICS_HPP

#include "defines.hpp"

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>

/**
 *	\note Declare and call from creation context (main)
 *		  extern void setup()
 *		  extern void teardown()
 */
namespace engine
{
namespace physics
{
	/**
	 *	steps physics engine forward
	 */
	void update();

	void create(const engine::Entity id, const BoxData & data);

	void create(const engine::Entity id, const CharacterData & data);

	void create(const engine::Entity id, const CylinderData & data);

	void create(const engine::Entity id, const SphereData & data);

	void remove(const engine::Entity id);

	void post_movement(engine::Entity id, core::maths::Vector3f movement);

	void post_heading(engine::Entity id, core::maths::radianf rotation);

	void query_position(const engine::Entity id, Vector3f & pos, Vector3f & vel, float & angle);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
