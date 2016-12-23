
#include <config.h>

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

#include "physics.hpp"

#include "actor_physx.hpp"
#include "helper_physx.hpp"
#include "material_physx.hpp"
#include "physics_physx.hpp"
#include "Callback.hpp"

#include <engine/graphics/renderer.hpp>

#include <core/container/CircleQueue.hpp>

namespace engine
{
namespace physics
{
	namespace
	{
		core::container::CircleQueueSRMW<std::pair<engine::Entity, BoxData>, 100> queue_create_boxes;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, CharacterData>, 100> queue_create_characters;
		core::container::CircleQueueSRMW<engine::Entity, 100> queue_remove;
	}

	/**
	 *	\note Helper method when creating dynamic objects
	 */
	physx::PxRigidDynamic * rigidDynamic(const physx::PxTransform & position, physx::PxShape *const shape, const float mass)
	{
		physx::PxRigidDynamic *const body = physx2::pWorld->createRigidDynamic(position);

		body->attachShape(*shape);

		physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);

		physx2::pScene->addActor(*body);

		return body;
	}
	/**
	 *	\note Helper method when creating static objects
	 */
	physx::PxRigidStatic * rigidStatic(const physx::PxTransform & position, physx::PxShape *const shape)
	{
		physx::PxRigidStatic *const body = physx2::pWorld->createRigidStatic(position);

		body->attachShape(*shape);

		physx2::pScene->addActor(*body);

		return body;
	}

	void create(const engine::Entity id, const BoxData & data)
	{
		const MaterialDef & materialDef = materials.at(data.material);

		physx::PxBoxGeometry geometry(convert<physx::PxVec3>(data.size));

		physx::PxShape *const shape = physx2::pWorld->createShape(geometry, *materialDef.material);

		switch (data.type)
		{
			case BodyType::DYNAMIC:
			{
				const float mass = materialDef.density * data.size.volume() * data.solidity;

				physx::PxRigidDynamic *const actor = rigidDynamic(convert<physx::PxTransform>(data.pos), shape, mass);
				actor->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				actors.emplace<ActorDynamic>(id, actor);
				break;
			}
			case BodyType::STATIC:
			{
				physx::PxRigidStatic *const actor = rigidStatic(convert<physx::PxTransform>(data.pos), shape);
				actor->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				actors.emplace<ActorStatic>(id, actor);
				break;
			}
			default:
				throw std::runtime_error("Box objects must be Dynamic or Static");
		}
	}

	void create(const engine::Entity id, const CharacterData & data)
	{
		const MaterialDef & materialDef = materials.at(data.material);

		physx::PxCapsuleControllerDesc desc;

		desc.height = data.height;
		desc.position = convert<physx::PxExtendedVec3>(data.pos);
		desc.radius = data.radius;
		desc.material = materialDef.material;

		if (!desc.isValid())
		{
			throw std::runtime_error("Controller description is not valid!");
		}

		physx::PxController *const controller = physx2::pControllerManager->createController(desc);

		if (controller==nullptr)
		{
			throw std::runtime_error("Could not create controller!");
		}

		controller->setUserData((void*) (std::size_t)static_cast<engine::Entity::value_type>(id));

		actors.emplace<ActorCharacter>(id, controller);
	}

	/**
	 *	\note Called during update.
	 */
	void update_start()
	{
		// poll remove queue
		{
			engine::Entity id;
			while (queue_remove.try_pop(id))
			{
				actors.remove(id);

				//// remove debug object from Renderer
				//if (actor.debugRenderId!= ::engine::Entity::INVALID)
				//	engine::graphics::renderer::remove(actor.debugRenderId);
			}
		}

		// poll boxes to create
		{
			std::pair<engine::Entity, BoxData> data;
			while (queue_create_boxes.try_pop(data))
			{
				create(data.first, data.second);
			}
		}

		// poll characters to create
		{
			std::pair<engine::Entity, CharacterData> data;
			while (queue_create_characters.try_pop(data))
			{
				create(data.first, data.second);
			}
		}
	}

	void post_create(const engine::Entity id, const BoxData & data)
	{
		queue_create_boxes.try_push(std::pair<engine::Entity, BoxData>(id, data));
	}

	void post_create(const engine::Entity id, const CharacterData & data)
	{
		queue_create_characters.try_push(std::pair<engine::Entity, CharacterData>(id, data));
	}

	void post_remove(const engine::Entity id)
	{
		queue_remove.try_push(id);
	}
}
}

#endif