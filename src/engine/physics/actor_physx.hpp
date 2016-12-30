
#ifndef ENGINE_PHYSICS_ACTOR_PHYSX_HPP
#define ENGINE_PHYSICS_ACTOR_PHYSX_HPP

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
	struct physx_guard
	{
		template <typename T>
		void operator () (T * data)
		{
			data->release();
		}
	};

	template<typename T>
	using physx_ptr = std::unique_ptr<T, physx_guard>;

	template<class T>
	struct Actor
	{
		physx_ptr<T> body;
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

	using ActorCollection = ::core::container::Collection
	<
		engine::Entity,
		500,
		std::array<ActorCharacter, 100>,
		std::array<ActorDynamic, 100>,
		std::array<ActorStatic, 100>
	>;

	// Collecation containing all Actors in the world.
	extern ActorCollection actors;
}
}

#endif /* ENGINE_PHYSICS_ACTOR_PHYSX_HPP */
