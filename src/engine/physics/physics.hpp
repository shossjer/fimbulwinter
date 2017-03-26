
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

	void update_joints();
	/**
	 *	\note steps physics engine forward
	 */
	void update_finish();

	void post_create(const engine::Entity id, const ActorData & data);

	void post_create(const engine::Entity id, const PlaneData & data);

	void post_remove(const engine::Entity id);

	struct joint_t
	{
		enum class Type
		{
			DISCONNECT,
			FIXED,
			HINGE
		};

		engine::Entity id;

		Type type;

		/**
		 \note can be "INVALID"-id if the second actor should be jointed with global space.
		 */
		engine::Entity actorId1;
		engine::Entity actorId2;

		transform_t transform1;
		transform_t transform2;

		float driveSpeed;
		float forceMax;
	};

	void post_joint(const joint_t & joint);

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

	/**
	 *	\note update Kinematic object with position and rotation
	 */
	void post_update_movement(const engine::Entity id, const transform_t translation);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
