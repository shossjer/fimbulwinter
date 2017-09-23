
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

		Actor(T *const body)
			:
			body(body)
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

	using ActorCollection = ::core::container::Collection
	<
		engine::Entity,
		600,
		std::array<ActorCharacter, 100>,
		std::array<ActorDynamic, 100>,
		std::array<ActorStatic, 100>
	>;

	// Collecation containing all Actors in the world.
	extern ActorCollection actors;

	template<class T>
	struct BaseJoint
	{
		physx_ptr<T> joint;

		BaseJoint(T *const joint)
			:
			joint(joint)
		{
		}
	};

	struct RevoluteJoint : BaseJoint<::physx::PxRevoluteJoint>
	{
		RevoluteJoint(::physx::PxRevoluteJoint * const joint) : BaseJoint(joint) {}
	};

	struct FixedJoint : BaseJoint<::physx::PxFixedJoint>
	{
		FixedJoint(::physx::PxFixedJoint * const joint) : BaseJoint(joint) {}
	};

	using JointCollection = ::core::container::Collection
	<
		engine::Entity,
		400,
		std::array<RevoluteJoint, 100>,
		std::array<FixedJoint, 100>
	>;

	extern JointCollection joints;
}
}

#endif /* ENGINE_PHYSICS_ACTOR_PHYSX_HPP */
