
#include "defines.hpp"

#include <array>
#include <vector>

namespace engine
{
namespace physics
{
	/**
	 *	get all Objects within radie of a pos
	 */
	void nearby(const Point & pos, const double radie, std::vector<Id> & objects);
	/**
	 *	steps physics engine forward
	 */
	void update();

	struct MoveData
	{
		const Vec2 velXZ;
		const double velY;

		MoveData(const Vec2 velXZ, const double velY)
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
	MoveResult update(const Id id, const MoveData & moveData);

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

	protected:

		ObjectData(const Point & pos, const Material material)
			:
			pos(pos), material(material)
		{}
	};

	struct BoxData : public ObjectData
	{
		const Size size;

		BoxData(const Point & pos, 
			const Material material, const Size & size)
			:
			ObjectData(pos, material),
			size(size)
		{}
	};

	void create(const Id id, const BoxData & data);

	struct CharacterData : public ObjectData
	{
		const float height;
		const float radius;

		CharacterData(const Point & pos, const Material material, const float height, const float radius)
			:
			ObjectData(pos, material),
			height(height),
			radius(radius)
		{}
	};

	void create(const Id id, const CharacterData & data);

	struct CylinderData : public ObjectData
	{
		const double height;
		const double radie;

		CylinderData(const Point & pos, const Material material, const double height, const double radie)
			:
			ObjectData(pos, material),
			height(height),
			radie(radie)
		{}
	};

	void create(const Id id, const CylinderData & data);

	struct SphereData : public ObjectData
	{
		const double radie;

		SphereData(const Point & pos, const Material material, const double radie)
			:
			ObjectData(pos, material),
			radie(radie)
		{}
	};

	void create(const Id id, const SphereData & data);

	void remove(const Id id);
}
}