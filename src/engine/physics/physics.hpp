
#ifndef ENGINE_PHYSICS_PHYSICS_HPP
#define ENGINE_PHYSICS_PHYSICS_HPP

#include "defines.hpp"

#include <core/maths/util.hpp>
#include <core/maths/Vector.hpp>

#include <array>
#include <vector>

namespace engine
{
namespace physics
{
	/**
	 *	steps physics engine forward
	 */
	void update();

	/**
	 */
	void post_movement(engine::Entity id, core::maths::Vector3f movement);
	/**
	 */
	void post_set_heading(engine::Entity id, core::maths::radianf rotation);

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

		const Point pos;
		const Material material;
		const float solidity;

	protected:

		ObjectData(const Point & pos, const Material material, const float solidity)
			:
			pos(pos), material(material), solidity(solidity)
		{}
	};

	struct BoxData : public ObjectData
	{
		const Size size;

		BoxData(const Point & pos, 
			const Material material, const float solidity, const Size & size)
			:
			ObjectData(pos, material, solidity),
			size(size)
		{}
	};

	void create(const engine::Entity id, const BoxData & data);

	struct CharacterData : public ObjectData
	{
		const float height;
		const float radius;

		CharacterData(const Point & pos, const Material material, const float solidity, const float height, const float radius)
			:
			ObjectData(pos, material, solidity),
			height(height),
			radius(radius)
		{}
	};

	void create(const engine::Entity id, const CharacterData & data);

	struct CylinderData : public ObjectData
	{
		const float height;
		const float radie;

		CylinderData(const Point & pos, const Material material, const float solidity, const float height, const float radie)
			:
			ObjectData(pos, material, solidity),
			height(height),
			radie(radie)
		{}
	};

	void create(const engine::Entity id, const CylinderData & data);

	struct SphereData : public ObjectData
	{
		const float radie;

		SphereData(const Point & pos, const Material material, const float solidity, const float radie)
			:
			ObjectData(pos, material, solidity),
			radie(radie)
		{}
	};

	void create(const engine::Entity id, const SphereData & data);

	void remove(const engine::Entity id);
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

#endif /* ENGINE_PHYSICS_PHYSICS_HPP */
