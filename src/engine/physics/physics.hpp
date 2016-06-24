
#include "defines.hpp"

#include <engine/Entity.hpp>

#include <array>
#include <vector>

namespace engine
{
namespace physics
{
	//class RayCast
	//{
	//public:
	//	virtual float callback(const Id id, const Point & point, const Point & normal, const float fraction) = 0;

	//	void execute(const Point & from, const Point & to);
	//};
	/**
	 *	get all Objects within radie of a pos
	 */
	void nearby(const Point & pos, const float radie, std::vector<engine::Entity> & objects);
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
