
#ifndef ENGINE_PHYSICS_MATERIAL_DATA_HPP
#define ENGINE_PHYSICS_MATERIAL_DATA_HPP

#include <config.h>

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

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

	// Material properties, used when creating actors
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
}
}

#endif // PHYSICS_USE_PHYSX

#endif // ENGINE_PHYSICS_MATERIAL_DATA_HPP