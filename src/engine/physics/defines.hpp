
#ifndef ENGINE_PHYSICS_DEFINES_HPP
#define ENGINE_PHYSICS_DEFINES_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>
#include <core/maths/Quaternion.hpp>

using Vector3f = core::maths::Vector3f;
using Quaternionf = core::maths::Quaternionf;

namespace engine
{
namespace physics
{
	constexpr float PHYSICS_PI = (float)3.1415926535897932384626433832795;

	enum class Material
	{
		LOW_FRICTION,
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

		union Geometry
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
					return (4.f / 3.f) * PHYSICS_PI * r*r*r;
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
				float size;
				float * p;
			} mesh;
		} geometry;

		//ShapeData()
		//{
		//}

		//ShapeData(const ShapeData & data)
		//	:
		//	type(data.type),
		//	material(data.material),
		//	solidity(data.solidity),
		//	pos(data.pos),
		//	rot(data.rot)
		//{
		//}

		//ShapeData(const Type type, const Material material, const float solidity, const Vector3f pos, const Quaternionf rot)
		//	:
		//	type(type),
		//	material(material),
		//	solidity(solidity),
		//	pos(pos),
		//	rot(rot),
		//	geometry(geometry)
		//{
		//}
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
			KINEMATIC
		} type;
		/**
		 *	/note Dictate how the Actor will be managed and in callbacks during collision.
		 */
		enum class Behaviour
		{
			// default behaviour will not receive any callbacks during collisions
			// walls and most simple dynamic actors belong here
			DEFAULT,
			// player will receive callback for all collisions.
			PLAYER,
			// obstacle will receive callback for all collisions.
			OBSTACLE,
			// trigger will receive trigger notifications always
			TRIGGER
		} behaviour;

		//Vector3f pos;
		float x, y, z;

		std::vector<ShapeData> shapes;

		//ActorData(const ActorData & data)
		//	:
		//	type(data.type),
		//	behaviour(data.behaviour),
		//	pos(data.pos),
		//	shapes(data.shapes)
		//{

		//}

		//ActorData()
		//{

		//}

		//ActorData(Type type, Behaviour behaviour, Vector3f pos, std::vector<ShapeData> shapes)
		//	:
		//	type(type),
		//	behaviour(behaviour),
		//	pos(pos),
		//	shapes(shapes)
		//{

		//}
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