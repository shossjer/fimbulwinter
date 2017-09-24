
#ifndef ENGINE_PHYSICS_PHYSICS_PHYSX_HPP
#define ENGINE_PHYSICS_PHYSICS_PHYSX_HPP

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

		// Cooking used during creation of Mesh shapes.
		extern ::physx::PxCooking * pCooking;

		// Manager for character controllers
		// used to create characters.
		extern ::physx::PxControllerManager * pControllerManager;
	}
}
}

#endif // ENGINE_PHYSICS_PHYSICS_PHYSX_HPP
