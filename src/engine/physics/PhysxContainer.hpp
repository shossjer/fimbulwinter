
#include <PxPhysicsAPI.h> 

#include <core/debug.hpp>

namespace engine
{
	class PhysxContainer
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

		PhysxContainer(const unsigned int numThreads = 1);

	public:

		void printError()
		{
		//	gDefaultErrorCallback
		}

		physx::PxScene * createScene();

		physx::PxMaterial * createMaterial()
		{
			// setup default material...
			physx::PxMaterial *const mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.1f);
			
			if (!mMaterial)
			{
				throw std::runtime_error("createMaterial failed!");
			}

			return mMaterial;
		}

		void createPlane(const physx::PxPlane & plane, physx::PxMaterial *const material, physx::PxScene *const scene)
		{
			scene->addActor(*physx::PxCreatePlane(*this->mPhysics, plane, *material));
		}

		physx::PxShape * createShape(const physx::PxGeometry &geometry, const physx::PxMaterial *const material)
		{
			// physx::PxBoxGeometry(2.f, 2.f, 2.f)
			physx::PxShape *const shape = mPhysics->createShape(geometry, *material);

			return shape;
		}

		physx::PxRigidDynamic * createRigidDynamic(const physx::PxTransform & position, physx::PxShape *const shape, physx::PxScene *const scene)
		{
			physx::PxRigidDynamic *const body = this->mPhysics->createRigidDynamic(position);

			body->attachShape(*shape);

			scene->addActor(*body);

			return body;
		}
	};
}