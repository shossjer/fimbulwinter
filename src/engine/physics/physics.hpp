
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
	namespace camera
	{
		struct Bounds
		{
			Vector3f min;
			Vector3f max;
		};

		/**
		 *	Sets the "bounds" used when updating an bounded Camera.
		 *
		 *	Should be based on the game level.
		 */
		void set(Bounds && bounds);
		/**
		 *	Register a Camera with an axis aligned bounding box
		 *
		 *	The bounding volume is set using the coordinate of the "min" corner
		 *	of the volume and the coordinate of the "max" corner of the volume.
		 */
		void add(engine::Entity camera, Vector3f position, bool bounded = true);
		/**
		 *	Update movement of the camera within its bounds.
		 */
		void update(engine::Entity camera, Vector3f movement);
	}

	struct orientation_movement
	{
		core::maths::Quaternionf quaternion;
	};

	/**
	 *	\note manages creation and removal of actors
	 */
	void update_start();

	void update_joints();
	/**
	 *	\note steps physics engine forward
	 */
	void update_finish();

	// TODO: make possible to add "definitions" and just directly create objects
	struct asset_definition_t
	{
	//	ActorData::Type type;
		ActorData::Behaviour behaviour;
		std::vector<ShapeData> shapes;
	};

	void add(const engine::Entity id, const asset_definition_t & data);

	struct asset_instance_t
	{
		Entity defId;
		transform_t transform;
		ActorData::Type type;
	};

	void add(const engine::Entity id, const asset_instance_t & data);

	void post_add_object(engine::Entity entity, engine::transform_t && data);

	void post_create(const engine::Entity id, const ActorData & data);

	void post_create(const engine::Entity id, const PlaneData & data);

	void post_remove(engine::Entity entity);

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
	void post_update_movement(engine::Entity entity, movement_data && data);

	/**
	 *	\note update Kinematic object with position and rotation
	 */
	void post_update_movement(const engine::Entity id, const transform_t translation);

	void post_update_orientation_movement(engine::Entity entity, orientation_movement && data);
	void post_update_transform(engine::Entity entity, engine::transform_t && data);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
