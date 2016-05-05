#include "PhysxContainer.hpp"

#include <iostream>

namespace engine
{
	PhysxContainer::PhysxContainer(const unsigned int numThreads)
		:
		gDefaultAllocatorCallback(),
		gDefaultErrorCallback()
	{
		this->mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, this->gDefaultAllocatorCallback, this->gDefaultErrorCallback);

		if (!this->mFoundation)
		{
			throw std::runtime_error("PxCreateFoundation failed!");
		}

		this->mProfileZoneManager = &physx::PxProfileZoneManager::createProfileZoneManager(this->mFoundation);

		if (!this->mProfileZoneManager)
		{
			throw std::runtime_error("PxProfileZoneManager failed!");
		}

		const bool recordMemoryAllocations = true;
		this->mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *this->mFoundation, physx::PxTolerancesScale(), recordMemoryAllocations, mProfileZoneManager);

		if (!this->mPhysics)
		{
			throw std::runtime_error("PxCreateFoundation failed!");
		}

		if (!PxInitExtensions(*this->mPhysics))
		{
			throw std::runtime_error("PxInitExtensions failed!");
		}

		mCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);

		if (!mCpuDispatcher)
		{
			throw std::runtime_error("PxDefaultCpuDispatcherCreate failed!");
		}
	}

	physx::PxScene * PhysxContainer::createScene()
	{
		physx::PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

		if (!sceneDesc.cpuDispatcher)
		{
			sceneDesc.cpuDispatcher = mCpuDispatcher;
		}

		if (!sceneDesc.filterShader)
		{
			sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		}

		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

		physx::PxScene *const mScene = mPhysics->createScene(sceneDesc);
		
		if (!mScene)
		{
			throw std::runtime_error("createScene failed!");
		}

		physx::PxSceneWriteLock scopedLock(*mScene);

		physx::PxSceneFlags flag = mScene->getFlags();

		PX_UNUSED(flag);
	//	mScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, false ? 1.0f : 0.0f);
	//	mScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);

		return mScene;
	}
}
