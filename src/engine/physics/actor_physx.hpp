
#ifndef ENGINE_PHYSICS_ACTOR_HPP
#define ENGINE_PHYSICS_ACTOR_HPP

#include <config.h>

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

#include <core/container/Collection.hpp>

#include <memory>

/**
 *	\note Should be used by physics implementation only.
 */
namespace engine
{
namespace physics
{
	constexpr unsigned int ACTORS_MAX = 500;
	constexpr unsigned int ACTORS_GROUP = 100;

	struct actor_delete
	{
		template <typename T>
		void operator () (T * data)
		{
			data->release();
		}
	};

	template<class T>
	struct Actor
	{
		std::unique_ptr<T, actor_delete> body;
		::engine::Entity debugRenderId;

		Actor(T *const body)
			:
			body(body),
			debugRenderId(engine::Entity::INVALID)
		{
		}
	};

	struct ActorCharacter : Actor<::physx::PxController>
	{
		/**
		 * Rotation along the y-axis (yaw).
		 *
		 * This value  is not used  by the underlaying  2D physics
		 * since  it  only  handles  rotations  along  the  z-axis
		 * (roll). It is however used in the modelview matrix sent
		 * to the renderer to help create the illusion of 3D.
		 */
		core::maths::radianf heading;

		ActorCharacter(::physx::PxController * const body)
			:
			Actor(body),
			heading(0.f)
		{
		}
	};

	struct ActorDynamic : Actor<::physx::PxRigidDynamic>
	{
		ActorDynamic(::physx::PxRigidDynamic * const body) : Actor(body) {}
	};

	struct ActorStatic : Actor<::physx::PxRigidStatic>
	{
		ActorStatic(::physx::PxRigidStatic * const body) : Actor(body) {}
	};

	// Collecation containing all Actors in the world.
	extern ::core::container::Collection
		<
		engine::Entity,
		ACTORS_MAX,
		std::array<ActorCharacter, ACTORS_GROUP>,
		std::array<ActorDynamic, ACTORS_GROUP>,
		std::array<ActorStatic, ACTORS_GROUP>
		>
		actors;
}
}

#endif /* PHYSICS_USE_PHYSX */

#endif /* ENGINE_PHYSICS_ACTOR_HPP */
