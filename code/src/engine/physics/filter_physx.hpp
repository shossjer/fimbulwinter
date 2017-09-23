
#ifndef ENGINE_PHYSICS_FILTER_PHYSX_HPP
#define ENGINE_PHYSICS_FILTER_PHYSX_HPP

#include <PxPhysicsAPI.h>

/**
*	\note Should be used by physics implementation only.
*/
namespace engine
{
namespace physics
{
	namespace filter
	{
		/**
			\note function used during contact filtering.
			
			The function determines which Actors will give contact notification.
		 */
		inline physx::PxFilterFlags collisionShader(physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
													physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
													physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize)
		{
			// http://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/AdvancedCollisionDetection.html

			// generate contacts for all that were not filtered above
			pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT;

			if ((filterData0.word0 & filterData1.word1)||(filterData1.word0 & filterData0.word1))
			{
				pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_FOUND;
				pairFlags |= physx::PxPairFlag::eNOTIFY_TOUCH_LOST;
			}

			return physx::PxFilterFlag::eDEFAULT;
		}
	}
}
}

#endif // ENGINE_PHYSICS_FILTER_PHYSX_HPP
