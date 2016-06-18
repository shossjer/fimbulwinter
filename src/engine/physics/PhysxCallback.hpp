 
#include "defines.hpp"

#include "PhysxScene.hpp"

namespace engine
{
namespace physics
{
	class PhysxCallback : public physx::PxSimulationEventCallback
	{
	private:

		PhysxScene & scene;

	public:

		PhysxCallback(PhysxScene & scene)
			:
			scene(scene)
		{
		}

	public:

		//	This is called when certain contact events occur.More...
		void onContact(const physx::PxContactPairHeader & pairHeader,
			const physx::PxContactPair *const pairs,
			const physx::PxU32 nbPairs) override
		{
			printf("\n");
		}

		//	This is called when a breakable constraint breaks.More...
		void onConstraintBreak(physx::PxConstraintInfo *const constraints, const physx::PxU32 count) override
		{

		}

		//	This is called with the actors which have just been woken up.More...
		void onWake(physx::PxActor **const actors, const physx::PxU32 count) override
		{
			for (unsigned int i = 0; i < count; i++)
			{
				printf("Object: %s is waking up\n", actors[i]->getName());
			}
		}

		//	This is called with the actors which have just been put to sleep.More...
		void onSleep(physx::PxActor **const actors, const physx::PxU32 count) override
		{
			for (unsigned int i = 0; i < count; i++)
			{
				printf("Object: %s is going to sleep ZZZzzz....\n", actors[i]->getName());
			}
		}

		//	This is called with the current trigger pair events.More...
		void onTrigger(physx::PxTriggerPair *const pairs, const physx::PxU32 count) override
		{

		}
	};
}
}
