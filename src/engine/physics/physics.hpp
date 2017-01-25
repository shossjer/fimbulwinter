
#ifndef ENGINE_PHYSICS_PHYSICS_HPP
#define ENGINE_PHYSICS_PHYSICS_HPP

#include "defines.hpp"

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/Matrix.hpp>

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

	void post_create(const engine::Entity id, const ActorData & data);

	void post_create(const engine::Entity id, const PlaneData & data);

	void post_remove(const engine::Entity id);

	struct movement_data
	{
		enum class Type
		{
			// value is multiplied with actors mass to get amount of force
			ACCELERATION,
			FORCE,
			IMPULSE,
			CHARACTER
		};

		Type type;
		core::maths::Vector3f vec;
	};
	/**
	 *	\note update Character or Dynamic object with delta movement or force.
	 */
	void post_update_movement(const engine::Entity id, const movement_data movement);

	struct translation_data
	{
		core::maths::Vector3f pos;
		core::maths::Quaternionf quat;
	};
	/**
	 *	\note update Kinematic object with position and rotation
	 */
	void post_update_movement(const engine::Entity id, const translation_data translation);

	void post_update_heading(const engine::Entity id, const core::maths::radianf rotation);

	void query_position(const engine::Entity id, Vector3f & pos, Quaternionf & rotation, Vector3f & vel);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
