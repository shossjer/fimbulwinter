 
#include "defines.hpp"

#include <physx/PxPhysicsAPI.h>

#include <core/debug.hpp>

#include <unordered_map>

namespace engine
{
namespace physics
{
	class PhysxScene
	{
	public:
		/**
		 *
		 */
		physx::PxScene *const instance;

		physx::PxControllerManager *const controllerManager;

//	private:
		/**
		 *
		 */
		std::unordered_map<Id, physx::PxRigidActor*> actors;
		std::unordered_map<Id, physx::PxController*> controllers;

	public:

		physx::PxRigidActor* actor(const Id id)
		{
			return this->actors.at(id);
		}

		physx::PxController* controller(const Id id)
		{
			return this->controllers.at(id);
		}

		void remove(const Id id)
		{
			auto t = this->actors.find(id);

			if (t != this->actors.end())
			{
				// 
				this->actors.erase(id);
			}
			else
			{
				// 
				this->controllers.erase(id);
			}
		}

	public:

		PhysxScene(physx::PxScene *const scene) : instance(scene), controllerManager(PxCreateControllerManager(*scene))
		{
			//physx::PxSceneDesc sceneDesc(sdk.get()->getTolerancesScale());
			//sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

			//if (!sceneDesc.cpuDispatcher)
			//{
			//	sceneDesc.cpuDispatcher = sdk.dispatcher();
			//}

			//if (!sceneDesc.filterShader)
			//{
			//	sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
			//}

			//sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

			//this->scene = sdk.get()->createScene(sceneDesc);

			//if (!this->scene)
			//{
			//	throw std::runtime_error("createScene failed!");
			//}

			//// Create Controller Manager
			//this->controllerManager = PxCreateControllerManager(*scene);
		}
	};
}
}
