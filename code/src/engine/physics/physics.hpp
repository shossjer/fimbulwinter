#pragma once

#include "core/maths/util.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"

#include "engine/physics/defines.hpp"
#include "engine/Token.hpp"

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
		void add(simulation & simulation, engine::Token camera, core::maths::Vector3f position, bool bounded = true);
		/**
		 *	Update movement of the camera within its bounds.
		 */
		void update(simulation & simulation, engine::Token camera, core::maths::Vector3f movement);
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

	void post_add_object(simulation & simulation, engine::Token entity, engine::transform_t && data);

	void post_remove(simulation & simulation, engine::Token entity);

	struct TransformComponents
	{
		uint16_t mask;
		uint16_t reserved;
		float values[9]; // todo
	};

	void set_transform_components(simulation & simulation, engine::Token entity, TransformComponents && data);
}
}
