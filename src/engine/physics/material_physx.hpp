
#ifndef ENGINE_PHYSICS_MATERIAL_DATA_HPP
#define ENGINE_PHYSICS_MATERIAL_DATA_HPP

#include <config.h>

#if PHYSICS_USE_PHYSX

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

	/**
	 *	\note Defined in physics_physx.cpp
	 */
	extern std::unordered_map<Material, MaterialDef> materials;
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

#endif // PHYSICS_USE_PHYSX

#endif // ENGINE_PHYSICS_MATERIAL_DATA_HPP