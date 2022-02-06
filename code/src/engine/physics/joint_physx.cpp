#include "config.h"

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

#include "physics.hpp"

#include "actor_physx.hpp"
#include "filter_physx.hpp"
#include "helper_physx.hpp"
#include "material_physx.hpp"
#include "physics_physx.hpp"
#include "Callback.hpp"

#include <core/container/CircleQueue.hpp>

namespace engine
{
namespace physics
{
	using namespace physx;

	namespace
	{
		core::container::CircleQueueSRMW<joint_t, 100> queue;
	}

	struct access
	{
		PxRigidActor * actor;

		access() : actor(nullptr) {}

		void operator () (ActorDynamic & x)
		{
			this->actor = x.body.get();
		}

		void operator () (ActorStatic & x)
		{
			this->actor = x.body.get();
		}

		template<typename X>
		void operator () (X & x)
		{
			debug_unreachable();
		}
	};

	void update_joints()
	{
		joint_t data;
		while (queue.try_pop(data))
		{
			access actor1;
			access actor2;

			if (data.actorId1!= engine::Entity::INVALID)
				actors.call(data.actorId1, actor1);
			actors.call(data.actorId2, actor2);

			switch (data.type)
			{
				case joint_t::Type::DISCONNECT:
				{
					break;
				}
				case joint_t::Type::HINGE:
				{
					auto joint = PxRevoluteJointCreate(
						*physx2::pWorld,
						actor1.actor, PxTransform{convert<PxVec3>(data.transform1.pos), convert(data.transform1.quat)},
						actor2.actor, PxTransform{convert<PxVec3>(data.transform2.pos), convert(data.transform2.quat)});

					if (data.id!= engine::Entity::INVALID) joints.add(data.id, joint);

					if (data.driveSpeed!=0.f)
					{
						joint->setDriveVelocity(data.driveSpeed);
						joint->setRevoluteJointFlag(PxRevoluteJointFlag::eDRIVE_ENABLED, true);

						if (data.forceMax!=0.f)
						{
							joint->setConstraintFlag(PxConstraintFlag::eDRIVE_LIMITS_ARE_FORCES, true);
							joint->setDriveForceLimit(5000);
						}
					}
					break;
				}
				case joint_t::Type::FIXED:
				{
					auto joint = PxFixedJointCreate(
						*physx2::pWorld,
						actor1.actor, PxTransform{convert<PxVec3>(data.transform1.pos), convert(data.transform1.quat)},
						actor2.actor, PxTransform{convert<PxVec3>(data.transform2.pos), convert(data.transform2.quat)});

					if (data.id!=engine::Entity::INVALID) joints.add(data.id, joint);
					break;
				}
				default:
				{
					break;
				}
			}
		}
	}

	void post_joint(const joint_t & joint)
	{
		const auto res = queue.try_push(joint);
		debug_assert(res);
	}
}
}

#endif
