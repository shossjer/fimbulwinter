 
#include "defines.hpp"

#include <physx/PxPhysicsAPI.h>

#include <core/debug.hpp>
#include <engine/Entity.hpp>

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
		// this is a hacky workaround  for entities and them not being
		// the same as integer types
		struct entityhash
		{
			std::size_t operator () (engine::Entity entity) const
			{
				return std::hash<std::size_t>{}(entity);
			}
		};
		/**
		 *
		 */
		std::unordered_map<engine::Entity, physx::PxRigidActor*, entityhash> actors;
		std::unordered_map<engine::Entity, physx::PxController*, entityhash> controllers;

	public:

		physx::PxRigidActor* actor(const engine::Entity id)
		{
			return this->actors.at(id);
		}

		physx::PxController* controller(const engine::Entity id)
		{
			return this->controllers.at(id);
		}

		void remove(const engine::Entity id)
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
