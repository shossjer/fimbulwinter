 
#include "defines.hpp"

#include <physx/PxPhysicsAPI.h>

#include <core/debug.hpp>

namespace engine
{
	class PhysxSDK
	{
	private:
		physx::PxDefaultErrorCallback gDefaultErrorCallback;
		physx::PxDefaultAllocator gDefaultAllocatorCallback;
		/**
		 *	Foundation object for Physx
		 */
		physx::PxFoundation * mFoundation;
		physx::PxProfileZoneManager * mProfileZoneManager;
		/**
		 *	The main Physx object
		 */
		physx::PxPhysics * mPhysics;

		physx::PxDefaultCpuDispatcher * mCpuDispatcher;

	public:

		physx::PxPhysics * get() { return this->mPhysics; }

		physx::PxDefaultCpuDispatcher * dispatcher() { return this->mCpuDispatcher; }
		
	public:

		PhysxSDK(const unsigned int numThreads = 1);

	public:

		physx::PxScene * createScene();

		physx::PxMaterial * createMaterial(const float friction, const float restitution)
		{
			// setup default material...
			physx::PxMaterial *const material = mPhysics->createMaterial(friction, friction, restitution);
			
			if (!material)
			{
				throw std::runtime_error("createMaterial failed!");
			}

			return material;
		}

		physx::PxRigidActor * createPlane(const physx::PxPlane & plane, physx::PxMaterial *const material, physx::PxScene *const scene)
		{
			physx::PxRigidStatic *const actor = physx::PxCreatePlane(*this->mPhysics, plane, *material);

			scene->addActor(*actor);

			return actor;
		}

		physx::PxShape * createShape(const physx::PxGeometry &geometry, const physx::PxMaterial *const material)
		{
			physx::PxShape *const shape = mPhysics->createShape(geometry, *material);
	
			return shape;
		}

		physx::PxRigidActor * createRigidDynamic(const physx::PxTransform & position, physx::PxShape *const shape, physx::PxScene *const scene, const float mass)
		{
			physx::PxRigidDynamic *const body = this->mPhysics->createRigidDynamic(position);

			body->attachShape(*shape);

			physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);

			scene->addActor(*body);

			return body;
		}
	};
}