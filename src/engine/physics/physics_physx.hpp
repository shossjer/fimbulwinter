
#ifndef ENGINE_PHYSICS_PHYSICS_PHYSX_HPP
#define ENGINE_PHYSICS_PHYSICS_PHYSX_HPP

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
		// PhysX Main instance
		// Most interaction is managed through Scenes in the Main instance.
		extern ::physx::PxPhysics * pWorld;

		// Actors are added to the PhysX Scene.
		// The Scene is similar to the Box2d world object.
		// Multiple Scene's can be added to PhysX but for now we will
		// stick to one.
		extern ::physx::PxScene * pScene;

		//::physx::PxDefaultErrorCallback gDefaultErrorCallback;
		//::physx::PxDefaultAllocator gDefaultAllocatorCallback;

		//::physx::PxFoundation * mFoundation;
		//::physx::PxProfileZoneManager * mProfileZoneManager;

		//::physx::PxDefaultCpuDispatcher * mCpuDispatcher;

		// Manager for character controllers
		// used to create characters.
		extern ::physx::PxControllerManager * pControllerManager;
	}
}
}

#endif // PHYSICS_USE_PHYSX

#endif // ENGINE_PHYSICS_PHYSICS_PHYSX_HPP
