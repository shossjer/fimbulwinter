
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
	enum class Material
	{
		LOW_FRICTION,
		MEETBAG,
		STONE,
		SUPER_RUBBER,
		WOOD
	};

	enum class BodyType
	{
		// special character type of dynamic body.
		// it is managed through a character controller which makes better
		// movement for characters.
		// when moving a Character body the Character move should be used.
		CHARACTER,
		// normal dynamic rigid bodies
		// when moving a Dynamic body the Force or Impulse move should be used.
		DYNAMIC,
		// static objects should never be moved.
		STATIC,
		// used for movable objects that are not affected by physics.
		// platforms and moving beams should be Kinematic.
		// when moving an Kinematic object the Kinematic move should be used.
		KINEMATIC
	};

	struct BoxData
	{
		BodyType type;
		Vector3f pos;
		Material material;
		float solidity;

		Vector3f size;
	};

	struct CharacterData
	{
		BodyType type;
		Vector3f pos;
		Material material;
		float solidity;

		float height;
		float radius;
	};

	struct CylinderData
	{
		BodyType type;
		Vector3f pos;
		Material material;
		float solidity;

		float height;
		float radie;
	};

	struct SphereData
	{
		BodyType type;
		Vector3f pos;
		Material material;
		float solidity;

		float radie;
	};

	struct PlaneData
	{
		BodyType type;
		Vector3f point;
		Vector3f normal;
		Material material;
	};
}
}

#endif