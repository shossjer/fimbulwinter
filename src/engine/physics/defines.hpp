
#ifndef ENGINE_PHYSICS_DEFINES_HPP
#define ENGINE_PHYSICS_DEFINES_HPP

#include <engine/Entity.hpp>

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/Quaternion.hpp>

#include <vector>

using Vector3f = core::maths::Vector3f;
using Quaternionf = core::maths::Quaternionf;

namespace engine
{
namespace physics
{
	enum class Material
	{
		LOW_FRICTION,
		OILY_ROBOT,
		MEETBAG,
		STONE,
		SUPER_RUBBER,
		WOOD
	};

	struct ShapeData
	{
		enum class Type
		{
			BOX,
			SPHERE,
			CAPSULE,
			MESH
		} type;

		Material material;
		// used during mass calculation
		// value should be: 0.f < x <= 1.f
		float solidity;

		Vector3f pos;
		Quaternionf rot;

		struct Geometry
		{
			struct Box
			{
				float w;
				float h;
				float d;

				float volume() const
				{
					return h*w*d;
				}

			} box;
			struct Sphere
			{
				float r;

				float volume() const
				{
					return (4.f/3.f) * core::maths::constantf::pi * r*r*r;
				}

			} sphere;
			struct Capsule
			{
				float w;
				float h;
				float d;
				float r;
			} capsule;
			struct Mesh
			{
				// xyz value of all points.
				std::vector<float> points;

				uint32_t size() const
				{
					debug_assert((points.size() % 3)== 0);
					return points.size()/3;
				}
			} mesh;

			Geometry(const Box & box) : box(box) {}
			Geometry(const Sphere & sphere) : sphere(sphere) {}
			Geometry(const Capsule & capsule) : capsule(capsule) {}
			Geometry(const Mesh & mesh) : mesh(mesh) {}

		} geometry;
	};

	struct ActorData
	{
		enum class Type
		{
			// static objects should never be moved.
			STATIC,
			// special character type of dynamic body.
			// it is managed through a character controller which makes better
			// movement for characters.
			// when moving a Character body the Character move should be used.
			// TODO: add back - CHARACTER,
			// normal dynamic rigid bodies
			// when moving a Dynamic body the Force or Impulse move should be used.
			DYNAMIC,
			// used for movable objects that are not affected by physics.
			// platforms and moving beams should be Kinematic.
			// when moving an Kinematic object the Kinematic move should be used.
			KINEMATIC,
			// special trigger object.
			// triggers are not movable and does not have a solid shape.
			// a trigger should always have trigger behaviour.
			TRIGGER
		} type;

		/**
			/note Dictate how the Actor will be managed and in callbacks during collision.

			During collisions the most "prio" behaviour object is reported as entity1.
			So if a Default box or a Player collides with a Trigger it will count as a
			trigger collision.
			Lower value is higher "prio".
		 */
		enum class Behaviour
		{
			// trigger will receive trigger notifications always
			// our gameplay can consider any object a "trigger", it does not need
			// to be a physics-trigger.
			// A visible pressure button/plate can be a static or kinematic or even
			// a dynamic object in the physics module.
			TRIGGER = 1<<0,
			// player will receive callback for all collisions.
			PLAYER = 1<<1,
			// obstacle will receive callback for all collisions.
			OBSTACLE = 1<<2,
			// default behaviour will not receive any callbacks during collisions
			// walls and most simple dynamic actors belong here
			DEFAULT = 1<<3
		} behaviour;

		Vector3f pos;
		Quaternionf rot;

		std::vector<ShapeData> shapes;
	};

	struct PlaneData
	{
		Vector3f point;
		Vector3f normal;
		Material material;
	};
}
}

#endif
