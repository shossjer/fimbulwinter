
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
	 *	\note manages creation and removal of actors
	 */
	void update_start();
	/**
	 *	\note steps physics engine forward
	 */
	void update_finish();

	void post_create(const engine::Entity id, const BoxData & data);

	void post_create(const engine::Entity id, const CharacterData & data);

	void post_remove(const engine::Entity id);

	void post_update_movement(const engine::Entity id, const core::maths::Vector3f movement);

	void post_update_heading(const engine::Entity id, const core::maths::radianf rotation);

	void query_position(const engine::Entity id, Vector3f & pos, Vector3f & vel, float & angle);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
