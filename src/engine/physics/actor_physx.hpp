
#ifndef ENGINE_PHYSICS_ACTOR_HPP
#define ENGINE_PHYSICS_ACTOR_HPP

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
	template<class T>
	struct Actor
	{
		T * body;
		::engine::Entity debugRenderId;
		/**
		 * Rotation along the y-axis (yaw).
		 *
		 * This value  is not used  by the underlaying  2D physics
		 * since  it  only  handles  rotations  along  the  z-axis
		 * (roll). It is however used in the modelview matrix sent
		 * to the renderer to help create the illusion of 3D.
		 */
		core::maths::radianf heading;

		Actor(T *const body)
			:
			body(body),
			debugRenderId(engine::Entity::INVALID),
			heading(0.f)
		{
		}
	};

	struct ActorCharacter : Actor<::physx::PxController>
	{
		ActorCharacter(::physx::PxController * const body) : Actor(body) {}
	};

	struct ActorDynamic : Actor<::physx::PxRigidDynamic>
	{
		ActorDynamic(::physx::PxRigidDynamic * const body) : Actor(body) {}
	};

	struct ActorStatic : Actor<::physx::PxRigidStatic>
	{
		ActorStatic(::physx::PxRigidStatic * const body) : Actor(body) {}
	};
}
}

#endif /* PHYSICS_USE_PHYSX */

#endif /* ENGINE_PHYSICS_ACTOR_HPP */
