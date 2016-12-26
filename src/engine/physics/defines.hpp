
#ifndef ENGINE_PHYSICS_DEFINES_HPP
#define ENGINE_PHYSICS_DEFINES_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>

using Vector3f = core::maths::Vector3f;

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
		CHARACTER,
		DYNAMIC,
		STATIC,
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