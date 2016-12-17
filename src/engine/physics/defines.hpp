
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
		MEETBAG,
		STONE,
		SUPER_RUBBER,
		WOOD
	};

	struct ObjectData
	{
		//enum Type
		//{
		//	STATIC,
		//	DYNAMIC,
		//	CHARACTER

		//}	type;

		const Vector3f pos;
		const Material material;
		const float solidity;

	protected:

		ObjectData(const Vector3f & pos, const Material material, const float solidity)
			:
			pos(pos), material(material), solidity(solidity)
		{}
	};

	struct BoxData : public ObjectData
	{
		const Vector3f size;

		BoxData(const Vector3f & pos,
			const Material material, const float solidity, const Vector3f & size)
			:
			ObjectData(pos, material, solidity),
			size(size)
		{}
	};

	struct CharacterData : public ObjectData
	{
		const float height;
		const float radius;

		CharacterData(const Vector3f & pos, const Material material, const float solidity, const float height, const float radius)
			:
			ObjectData(pos, material, solidity),
			height(height),
			radius(radius)
		{}
	};

	struct CylinderData : public ObjectData
	{
		const float height;
		const float radie;

		CylinderData(const Vector3f & pos, const Material material, const float solidity, const float height, const float radie)
			:
			ObjectData(pos, material, solidity),
			height(height),
			radie(radie)
		{}
	};

	struct SphereData : public ObjectData
	{
		const float radie;

		SphereData(const Vector3f & pos, const Material material, const float solidity, const float radie)
			:
			ObjectData(pos, material, solidity),
			radie(radie)
		{}
	};
}
}

namespace std
{
	template<> struct hash<engine::physics::Material>
	{
		std::size_t operator () (const engine::physics::Material material) const
		{
			return std::hash<std::size_t>{}(static_cast<std::size_t>(material));
		}
	};
}

#endif