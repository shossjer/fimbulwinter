
#ifndef ENGINE_PHYSICS_MATERIAL_PHYSX_HPP
#define ENGINE_PHYSICS_MATERIAL_PHYSX_HPP

#include <PxPhysicsAPI.h>

#include <unordered_map>

/**
 *	\note Should be used by physics implementation only.
 */
namespace engine
{
namespace physics
{
	namespace physx2
	{
		extern ::physx::PxPhysics * pWorld;
	}

	/**
	 *	\note Material properties, used when creating actors
	 */
	struct MaterialDef
	{
		const float density;
		::physx::PxMaterial *const material;

		MaterialDef(const float density, const float friction, const float restitution)
			:
			density(density),
			material(physx2::pWorld->createMaterial(friction, friction, restitution))
		{
		}
	};

	using MaterialMap = std::unordered_map<Material, MaterialDef>;

	/**
	 *	\note Defined in physics_physx.cpp
	 */
	extern MaterialMap materials;
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

#endif // ENGINE_PHYSICS_MATERIAL_PHYSX_HPP
