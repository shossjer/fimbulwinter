
#include <config.h>

#include "PhysxScene.hpp"

#include "defines.hpp"

#include <unordered_map>

namespace engine
{
namespace physics
{
	void nearby(const PhysxScene & scene, const physx::PxVec3 & pos, const double radie, std::vector<Id> & objects)
	{
		const physx::PxU32 bufferSize = 32;
		physx::PxOverlapHit overlapBuffer[bufferSize];

		physx::PxOverlapBuffer result{overlapBuffer, bufferSize};

	//	physx::PxQueryFilterData filter{ physx::PxQueryFlags{physx::PxQueryFlag::} };
		
		if (scene.instance->overlap(physx::PxSphereGeometry(radie), physx::PxTransform(pos), result))//, filter))
		{
			const unsigned int numCollisions = result.getNbTouches();
			
			for (unsigned int i = 0; i < numCollisions; i++)
			{
				const physx::PxOverlapHit & hit = result.getAnyHit(i);

				objects.push_back(reinterpret_cast<Id>(hit.actor->userData));
			}
		}
	}
}
}
