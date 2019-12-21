
#ifndef ENGINE_PHYSICS_PHYSICS_HPP
#define ENGINE_PHYSICS_PHYSICS_HPP

#include "defines.hpp"

#include "core/maths/util.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/Matrix.hpp"

#include "engine/Entity.hpp"

namespace engine
{
	namespace graphics
	{
		class renderer;
		class viewer;
	}
}

/**
 *	\note Declare and call from creation context (main)
 *		  extern void setup()
 *		  extern void teardown()
 */
namespace engine
{
namespace physics
{
	class simulation
	{
	public:
		~simulation();
		simulation(engine::graphics::renderer & renderer, engine::graphics::viewer & viewer);
	};

	namespace camera
	{
		struct Bounds
		{
			core::maths::Vector3f min;
			core::maths::Vector3f max;
		};

		/**
		 *	Sets the "bounds" used when updating an bounded Camera.
		 *
		 *	Should be based on the game level.
		 */
		void set(simulation & simulation, Bounds && bounds);
		/**
		 *	Register a Camera with an axis aligned bounding box
		 *
		 *	The bounding volume is set using the coordinate of the "min" corner
		 *	of the volume and the coordinate of the "max" corner of the volume.
		 */
		void add(simulation & simulation, engine::Entity camera, core::maths::Vector3f position, bool bounded = true);
		/**
		 *	Update movement of the camera within its bounds.
		 */
		void update(simulation & simulation, engine::Entity camera, core::maths::Vector3f movement);
	}

	struct orientation_movement
	{
		core::maths::Quaternionf quaternion;
	};

	/**
	 *	\note manages creation and removal of actors
	 */
	void update_start(simulation & simulation);

	void update_joints(simulation & simulation);
	/**
	 *	\note steps physics engine forward
	 */
	void update_finish(simulation & simulation);

	// TODO: make possible to add "definitions" and just directly create objects
	struct asset_definition_t
	{
	//	ActorData::Type type;
		ActorData::Behaviour behaviour;
		std::vector<ShapeData> shapes;
	};

	void add(simulation & simulation, const engine::Entity id, const asset_definition_t & data);

	struct asset_instance_t
	{
		Entity defId;
		transform_t transform;
		ActorData::Type type;
	};

	void add(simulation & simulation, const engine::Entity id, const asset_instance_t & data);

	void post_add_object(simulation & simulation, engine::Entity entity, engine::transform_t && data);

	void post_create(simulation & simulation, const engine::Entity id, const ActorData & data);

	void post_create(simulation & simulation, const engine::Entity id, const PlaneData & data);

	void post_remove(simulation & simulation, engine::Entity entity);

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

	void post_joint(simulation & simulation, const joint_t & joint);

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
	void post_update_movement(simulation & simulation, engine::Entity entity, movement_data && data);

	/**
	 *	\note update Kinematic object with position and rotation
	 */
	void post_update_movement(simulation & simulation, const engine::Entity id, const transform_t translation);

	void post_update_orientation_movement(simulation & simulation, engine::Entity entity, orientation_movement && data);
	void post_update_transform(simulation & simulation, engine::Entity entity, engine::transform_t && data);
}
}

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
