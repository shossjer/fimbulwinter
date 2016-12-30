
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
		using PxU32 = physx::PxU32;
		using PxFilterData = physx::PxFilterData;

		enum class Flag
		{
			TRIGGER = (1<<0),
			PLAYER = (1<<1),
			OBSTACLE = (1<<2),
			STATIC = (1<<3),
			DYNAMIC = (1<<4)
		};

		/**
			\note function used during contact filtering.
			
			The function determines which Actors will give contact notification.
		 */
		inline physx::PxFilterFlags collisionShader(physx::PxFilterObjectAttributes attributes0, PxFilterData filterData0,
													physx::PxFilterObjectAttributes attributes1, PxFilterData filterData1,
													physx::PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
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

		/**
			\note Create Filter for Dynamic Actor based on behaviour
		 */
		inline PxFilterData dynamic(const ActorData & data)
		{
			PxU32 flag;
			PxU32 mask;

			switch (data.behaviour)
			{
				// players should have full callback
				case ActorData::Behaviour::PLAYER:
				flag =	static_cast<PxU32>(Flag::PLAYER);
				mask =	static_cast<PxU32>(Flag::DYNAMIC)|
						static_cast<PxU32>(Flag::PLAYER)|
						static_cast<PxU32>(Flag::OBSTACLE)|
						static_cast<PxU32>(Flag::STATIC);
				break;

				// no callback is needed for dynamic default objects
				case ActorData::Behaviour::DEFAULT:
				flag = static_cast<PxU32>(Flag::DYNAMIC);
				mask = static_cast<PxU32>(0);
				break;

				default:
				// Behaviour is not valid for Actor type
				debug_unreachable();
			}

			return PxFilterData {flag, mask, PxU32 {0}, PxU32 {0}};
		}

		/**
			\note Create Filter for Kinematic Actor based on behaviour
		 */
		inline PxFilterData kinematic(const ActorData & data)
		{
			PxU32 flag;
			PxU32 mask;

			switch (data.behaviour)
			{
				// obstacles should have full callback (temp disabled)
				case ActorData::Behaviour::OBSTACLE:
				flag =	static_cast<PxU32>(Flag::OBSTACLE);
				mask = 0;
				/*static_cast<PxU32>(Flag::DYNAMIC)|
						static_cast<PxU32>(Flag::PLAYER)|
						static_cast<PxU32>(Flag::OBSTACLE)|
						static_cast<PxU32>(Flag::STATIC);*/
				break;

				// players should have full callback
				case ActorData::Behaviour::PLAYER:
				flag =	static_cast<PxU32>(Flag::PLAYER);
				mask =	static_cast<PxU32>(Flag::DYNAMIC)|
						static_cast<PxU32>(Flag::PLAYER)|
						static_cast<PxU32>(Flag::OBSTACLE)|
						static_cast<PxU32>(Flag::STATIC);
				break;

				// no callback is needed for dynamic default objects
				case ActorData::Behaviour::DEFAULT:
				flag = static_cast<PxU32>(Flag::DYNAMIC);
				mask = static_cast<PxU32>(0);
				break;

				default:
				// Behaviour is not valid for Actor type
				debug_unreachable();
			}

			return PxFilterData {flag, mask, PxU32 {0}, PxU32 {0}};
		}

		/**
			\note Create Filter for Static Actor based on behaviour
		 */
		inline PxFilterData fixed(const ActorData & data)
		{
			PxU32 flag;
			PxU32 mask;

			switch (data.behaviour)
			{
				// no callback is needed for static default objects
				case ActorData::Behaviour::DEFAULT:
				flag = static_cast<PxU32>(Flag::STATIC);
				mask = static_cast<PxU32>(0);
				break;

				default:
				// Behaviour is not valid for Actor type
				debug_unreachable();
			}

			return PxFilterData {flag, mask, PxU32 {0}, PxU32 {0}};
		}
	}
}
}

#endif // ENGINE_PHYSICS_FILTER_PHYSX_HPP
