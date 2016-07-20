
#include "defines.hpp"

#include <engine/Entity.hpp>

#include <array>
#include <vector>

namespace engine
{
namespace physics
{
	/**	 
	 *
	 */
	Point load(const engine::Entity id);
	/**
	 *	steps physics engine forward
	 */
	void update();

	struct MoveData
	{
		const Vec2 velXZ;
		const float velY;

		MoveData(const Vec2 velXZ, const float velY)
			:
			velXZ(velXZ), velY(velY)
		{}
	};

	struct MoveResult
	{
		const bool grounded;
		const Point pos;
		const float velY;

		MoveResult(const bool grounded, const Point & pos, const float velY)
			: 
			grounded(grounded), pos(pos), velY(velY)
		{}
	};
	/**
	 *	updates a single character Object with Movement data
	 */
	MoveResult update(const engine::Entity id, const MoveData & moveData);

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
