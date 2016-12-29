
#include <config.h>

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

#include "physics.hpp"

#include "actor_physx.hpp"
#include "filter_physx.hpp"
#include "helper_physx.hpp"
#include "material_physx.hpp"
#include "Callback.hpp"

#include <engine/graphics/renderer.hpp>

#include <core/container/CircleQueue.hpp>
#include <core/maths/Quaternion.hpp>

#include <unordered_map>

namespace engine
{
namespace physics
{
	constexpr float TIME_STEP = 1.f/60.f;

	using namespace physx;

	namespace physx2
	{
		// PhysX Main instance
		// Most interaction is managed through Scenes in the Main instance.
		::physx::PxPhysics * pWorld;

		// Actors are added to the PhysX Scene.
		// The Scene is similar to the Box2d world object.
		// Multiple Scene's can be added to PhysX but for now we will
		// stick to one.
		::physx::PxScene * pScene;

		::physx::PxDefaultErrorCallback gDefaultErrorCallback;
		::physx::PxDefaultAllocator gDefaultAllocatorCallback;

		::physx::PxFoundation * mFoundation;
		::physx::PxProfileZoneManager * mProfileZoneManager;

		::physx::PxDefaultCpuDispatcher * mCpuDispatcher;

		// Manager for character controllers
		// used to create characters.
		::physx::PxControllerManager * pControllerManager;
	}

	// Collecation containing all Actors in the world.
	::core::container::Collection
		<
		engine::Entity,
		ACTORS_MAX,
		std::array<ActorCharacter, ACTORS_GROUP>,
		std::array<ActorDynamic, ACTORS_GROUP>,
		std::array<ActorStatic, ACTORS_GROUP>
		>
		actors;

	// All defined physics materials. Contains density, friction and restitution
	std::unordered_map<Material, MaterialDef> materials;

	namespace
	{
		// Callback instance for engine notifications
		//	* onFalling - called when Actor start falling
		//	* onGrounded - called when Actor is grounded after falling
		const Callback * pCallback;

		core::container::CircleQueueSRMW<std::pair<engine::Entity, core::maths::radianf>, 100> queue_headings;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, movement_data>, 100> queue_movements;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, translation_data>, 100> queue_translations;
	}

	class SimulationEventCallback : public physx::PxSimulationEventCallback
	{
		void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count)
		{
		}

		void onWake(physx::PxActor** actors, physx::PxU32 count)
		{
		}

		void onSleep(physx::PxActor** actors, physx::PxU32 count)
		{
		}

		void onContact(const physx::PxContactPairHeader & pairHeader, const physx::PxContactPair * pairs, physx::PxU32 nbPairs)
		{
			auto val = pairs[0];

			if (val.events.isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND))
			{
				Entity ids[2];
				Material materials[2];

				ids[0] = (std::size_t)(pairHeader.actors[0]->userData);
				ids[1] = (std::size_t)(pairHeader.actors[1]->userData);

				materials[0] = static_cast<Material>(pairs[0].shapes[0]->getSimulationFilterData().word2);
				materials[1] = static_cast<Material>(pairs[0].shapes[1]->getSimulationFilterData().word2);

				pCallback->postContactFound(ids, materials);
			}
			else
			if (val.events.isSet(PxPairFlag::eNOTIFY_TOUCH_LOST))
			{
				Entity ids[2];

				ids[0] = (std::size_t)(pairHeader.actors[0]->userData);
				ids[1] = (std::size_t)(pairHeader.actors[1]->userData);

				pCallback->postContactLost(ids);
			}
			//PxContactPairHeaderFlags;//pairHeader.flags
		}

		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
		{
		}
	} simulationCallback;

	/**
	 * \note Declared and called from main.
	 */
	void setup()
	{
		physx2::mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, physx2::gDefaultAllocatorCallback, physx2::gDefaultErrorCallback);

		if (!physx2::mFoundation)
		{
			throw std::runtime_error("PxCreateFoundation failed!");
		}

		physx2::mProfileZoneManager = &physx::PxProfileZoneManager::createProfileZoneManager(physx2::mFoundation);

		if (!physx2::mProfileZoneManager)
		{
			throw std::runtime_error("PxProfileZoneManager failed!");
		}

		const bool recordMemoryAllocations = true;
		physx2::pWorld = PxCreatePhysics(PX_PHYSICS_VERSION, *physx2::mFoundation, physx::PxTolerancesScale(), recordMemoryAllocations, physx2::mProfileZoneManager);

		if (!physx2::pWorld)
		{
			throw std::runtime_error("PxCreateFoundation failed!");
		}

		if (!PxInitExtensions(*physx2::pWorld))
		{
			throw std::runtime_error("PxInitExtensions failed!");
		}

		physx2::mCpuDispatcher = physx::PxDefaultCpuDispatcherCreate(1);

		if (!physx2::mCpuDispatcher)
		{
			throw std::runtime_error("PxDefaultCpuDispatcherCreate failed!");
		}

		// Create just one Scene for the PhysX world for now.
		physx::PxSceneDesc sceneDesc(physx2::pWorld->getTolerancesScale());
		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);

		if (!sceneDesc.cpuDispatcher)
		{
			sceneDesc.cpuDispatcher = physx2::mCpuDispatcher;
		}

		sceneDesc.filterShader = filter::collisionShader;

		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

		physx2::pScene = physx2::pWorld->createScene(sceneDesc);

		if (!physx2::pScene)
		{
			throw std::runtime_error("createScene failed!");
		}

		// Create the Manager for character controllers used to create characters.
		physx2::pControllerManager = PxCreateControllerManager(*physx2::pScene);

		// Water (salt)	1,030
		// Plastics		1,175
		// Concrete		2,000
		// Iron			7, 870
		// Lead			11,340
		// setup materials
		materials.emplace(Material::LOW_FRICTION, MaterialDef(1000.f, .0f, .0f));

		materials.emplace(Material::OILY_ROBOT, MaterialDef(1000.f, .0f, .4f));
		materials.emplace(Material::MEETBAG, MaterialDef(1000.f, .5f, .4f));

		materials.emplace(Material::STONE, MaterialDef(2000.f, .4f, .05f));
		materials.emplace(Material::SUPER_RUBBER, MaterialDef(1200.f, 1.0f, 1.0f));
		materials.emplace(Material::WOOD, MaterialDef(700.f, .6f, .2f));

		physx2::pScene->setSimulationEventCallback(&simulationCallback);
	}

	/**
	 * \note Declared and called from main.
	 */
	void teardown()
	{
		physx2::pControllerManager->release();

		physx2::pScene->release();

		physx2::pWorld->release();

		PxCloseExtensions();

		physx2::mCpuDispatcher->release();

		physx2::mProfileZoneManager->release();

		physx2::mFoundation->release();
	}

	struct actor_mover
	{
		const movement_data movement;

		actor_mover(const movement_data movement) : movement(movement) {}

		void operator () (ActorCharacter & x)
		{
			// movement needs to be rotated acording to character heading.. hope this can be done better
			core::maths::Vector3f::array_type buffer;
			movement.vec.get_aligned(buffer);

			// is this really the scale? /8
			const float mx = -buffer[1]*std::sin(x.heading.get())/8;
			const float my = buffer[2]/8;

			const auto val = physx::PxVec3 {mx, my + 0.5f*(TIME_STEP*-9.82f), 0.f};

			x.body->move(val, 0.0f, TIME_STEP, physx::PxControllerFilters {});
		}

		void operator () (ActorDynamic & x)
		{
			core::maths::Vector3f::array_type buffer;
			movement.vec.get_aligned(buffer);

			switch (this->movement.type)
			{
				case movement_data::Type::ACCELERATION:
				{
					x.body->addForce(convert<physx::PxVec3>(this->movement.vec*x.body->getMass()), physx::PxForceMode::eFORCE);
					break;
				}
				case movement_data::Type::IMPULSE:
				{
					x.body->addForce(convert<physx::PxVec3>(this->movement.vec), physx::PxForceMode::eIMPULSE);
					break;
				}
				case movement_data::Type::FORCE:
				{
					x.body->addForce(convert<physx::PxVec3>(this->movement.vec), physx::PxForceMode::eFORCE);
					break;
				}
			}
		}

		void operator () (ActorStatic & x)
		{
			// should not move
			debug_unreachable();
		}
	};

	struct actor_translate
	{
		const translation_data translation;

		actor_translate(const translation_data translation) : translation(translation) {}

		void operator () (ActorCharacter & x)
		{
			debug_unreachable();
		}

		void operator () (ActorDynamic & x)
		{
			// make sure object is Kinematic
			debug_assert(x.body->getRigidBodyFlags().isSet(physx::PxRigidBodyFlag::eKINEMATIC));

			physx::PxTransform t {convert<physx::PxVec3>(this->translation.pos), convert(this->translation.quat) };

			x.body->setKinematicTarget(t);
		}

		void operator () (ActorStatic & x)
		{
			// should not move
			debug_unreachable();
		}
	};

	struct actor_header
	{
		const core::maths::radianf heading;

		actor_header(const core::maths::radianf heading) : heading(heading) {}

		void operator () (ActorCharacter & x)
		{
			x.heading = heading;
		}

		template<typename X>
		void operator () (X & x)
		{
			debug_unreachable();
		}
	};

	void update_finish()
	{
		// poll heading queue
		{
			std::pair<engine::Entity, core::maths::radianf> data;
			while (queue_headings.try_pop(data))
			{
				actors.call(data.first, actor_header {data.second});
			}
		}
		// poll movement queue
		{
			std::pair<engine::Entity, movement_data> data;
			while (queue_movements.try_pop(data))
			{
				actors.call(data.first, actor_mover {data.second});
			}
		}
		// poll translation queue
		{
			std::pair<engine::Entity, translation_data> data;
			while (queue_translations.try_pop(data))
			{
				actors.call(data.first, actor_translate {data.second});
			}
		}

		// Update the physics world
		physx2::pScene->simulate(TIME_STEP);
		physx2::pScene->fetchResults(true);

		// retrieve array of actors that moved
		physx::PxU32 nbActiveTransforms;
		const physx::PxActiveTransform* activeTransforms = physx2::pScene->getActiveTransforms(nbActiveTransforms);

		// update each render object with the new transform
		for (physx::PxU32 i = 0; i < nbActiveTransforms; ++i)
		{
			const auto item = activeTransforms[i];

			auto id = engine::Entity {static_cast<engine::Entity::value_type>((std::size_t)item.userData)};

			const auto pose = item.actor2World;

			engine::graphics::data::ModelviewMatrix data = {
				core::maths::Matrix4x4f::translation(pose.p.x, pose.p.y, pose.p.z) *
				make_matrix(core::maths::Quaternionf(-pose.q.w, pose.q.x, pose.q.y, pose.q.z))
			};

			engine::graphics::renderer::update(id, std::move(data));

		//	if (actor.debugRenderId!=::engine::Entity::INVALID)
		//		engine::graphics::renderer::update(actor.debugRenderId, std::move(data));
		}
	}

	/**
	 * \note Declared and called from gameplay.
	 */
	void subscribe(const Callback & callback)
	{
		pCallback = &callback;
	}

	struct get_transformation
	{
		core::maths::Vector3f position;
		core::maths::Quaternionf quaternion;
		core::maths::Vector3f velocity;

		void operator () (const ActorCharacter & x)
		{
			const auto body = x.body.get();

			this->position = convert(body->getPosition());
			this->velocity = convert(body->getActor()->getLinearVelocity());
		}

		void operator () (const ActorDynamic & x)
		{
			const auto body = x.body.get();
			const auto trans = body->getGlobalPose();

			this->position = convert(trans.p);
			this->quaternion = Quaternionf { trans.q.w, trans.q.x, trans.q.y, trans.q.z};
			this->velocity = convert(body->getLinearVelocity());
		}

		void operator () (const ActorStatic & x)
		{
		}
	};

	void query_position(const engine::Entity id, Vector3f & position, Quaternionf & quaternion, Vector3f & velocity)
	{
		get_transformation data {};

		actors.call(id, data);

		position = data.position;
		quaternion = data.quaternion;
		velocity = data.velocity;
	}

	void post_update_movement(const engine::Entity id, const movement_data movement)
	{
		const auto res = queue_movements.try_emplace(id, movement);
		debug_assert(res);
	}
	void post_update_movement(const engine::Entity id, const translation_data translation)
	{
		const auto res = queue_translations.try_emplace(id, translation);
		debug_assert(res);
	}
	void post_update_heading(const engine::Entity id, const core::maths::radianf rotation)
	{
		const auto res = queue_headings.try_emplace(id, rotation);
		debug_assert(res);
	}
}
}

#endif
