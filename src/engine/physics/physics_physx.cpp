
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

		::physx::PxFoundation * pFoundation;
		::physx::PxCooking * pCooking;

		::physx::PxDefaultCpuDispatcher * pCpuDispatcher;
	}

	// Collecation containing all Actors in the world.
	ActorCollection actors;

	// All defined physics materials. Contains density, friction and restitution
	MaterialMap materials;

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
			debug_printline(0xffffffff, "onConstraintBreak");
		}

		void onWake(physx::PxActor** actors, physx::PxU32 count)
		{
			debug_printline(0xffffffff, "onWake");
		}

		void onSleep(physx::PxActor** actors, physx::PxU32 count)
		{
			debug_printline(0xffffffff, "onSleep");
		}

		void onContact(const physx::PxContactPairHeader & pairHeader, const physx::PxContactPair * pairs, physx::PxU32 nbPairs)
		{
			const auto val = pairs[0];

			Callback::data_t data;

			const auto & filterData1 = val.shapes[0]->getSimulationFilterData();
			const auto & filterData2 = val.shapes[1]->getSimulationFilterData();

			// find most prio object behaviour
			if (filterData1.word0 < filterData2.word0)
			{
				data.ids[0] = (std::size_t)(pairHeader.actors[0]->userData);
				data.ids[1] = (std::size_t)(pairHeader.actors[1]->userData);

				data.behaviours[0] = static_cast<ActorData::Behaviour>(filterData1.word0);
				data.behaviours[1] = static_cast<ActorData::Behaviour>(filterData2.word0);

				data.materials[0] = static_cast<Material>(filterData1.word2);
				data.materials[1] = static_cast<Material>(filterData2.word2);
			}
			else
			{
				data.ids[1] = (std::size_t)(pairHeader.actors[0]->userData);
				data.ids[0] = (std::size_t)(pairHeader.actors[1]->userData);

				data.behaviours[1] = static_cast<ActorData::Behaviour>(filterData1.word0);
				data.behaviours[0] = static_cast<ActorData::Behaviour>(filterData2.word0);

				data.materials[1] = static_cast<Material>(filterData1.word2);
				data.materials[0] = static_cast<Material>(filterData2.word2);
			}

			if (val.events.isSet(PxPairFlag::eNOTIFY_TOUCH_FOUND))
			{
				pCallback->postContactFound(data);
			}
			else
			if (val.events.isSet(PxPairFlag::eNOTIFY_TOUCH_LOST))
			{
				pCallback->postContactLost(data);
			}
		}

		void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
		{
			const auto val = pairs[0];

			Callback::data_t data;

			data.ids[0] = (std::size_t)(val.triggerActor->userData);
			data.ids[1] = (std::size_t)(val.otherActor->userData);

			const auto & filterData1 = val.triggerShape->getSimulationFilterData();
			const auto & filterData2 = val.otherShape->getSimulationFilterData();

			data.behaviours[0] = static_cast<ActorData::Behaviour>(filterData1.word0);
			data.behaviours[1] = static_cast<ActorData::Behaviour>(filterData2.word0);

			data.materials[0] = static_cast<Material>(filterData1.word2);
			data.materials[1] = static_cast<Material>(filterData2.word2);

			if (val.status==PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				pCallback->postTriggerFound(data);
			}
			else
			{
				pCallback->postTriggerLost(data);
			}
		}
	} simulationCallback;

	/**
	 * \note Declared and called from main.
	 */
	bool setup()
	{
		physx_ptr<physx::PxFoundation> pFoundation {PxCreateFoundation(PX_PHYSICS_VERSION, physx2::gDefaultAllocatorCallback, physx2::gDefaultErrorCallback)};

		if (pFoundation.get()==nullptr)
		{
			debug_printline(0xffffffff, "Could not create physx Foundation.");
			return false;
		}

		physx_ptr<physx::PxPhysics> pWorld {PxCreatePhysics(PX_PHYSICS_VERSION, *pFoundation, physx::PxTolerancesScale())};

		if (pWorld.get()==nullptr)
		{
			debug_printline(0xffffffff, "Could not create physx World.");
			return false;
		}

		if (!PxInitExtensions(*pWorld))
		{
			debug_printline(0xffffffff, "Could not init extensions.");
			return false;
		}

		physx_ptr<physx::PxDefaultCpuDispatcher> pCPUDispatcher {physx::PxDefaultCpuDispatcherCreate(1)};

		if (pCPUDispatcher.get()==nullptr)
		{
			debug_printline(0xffffffff, "Could not create physx DefaultCpuDispatcher.");
			return false;
		}

		// Create just one Scene for the PhysX world for now.
		physx::PxSceneDesc sceneDesc(pWorld->getTolerancesScale());

		sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
		sceneDesc.cpuDispatcher = pCPUDispatcher.get();
		sceneDesc.filterShader = filter::collisionShader;
		sceneDesc.flags |= physx::PxSceneFlag::eENABLE_ACTIVETRANSFORMS;

		physx_ptr<physx::PxScene> pScene {pWorld->createScene(sceneDesc)};

		if (pScene.get()==nullptr)
		{
			debug_printline(0xffffffff, "Could not create physx Scene.");
			return false;
		}

		physx::PxCookingParams cookingParams {physx::PxTolerancesScale()};
		physx_ptr<physx::PxCooking> pCooking {PxCreateCooking(PX_PHYSICS_VERSION, *pFoundation, cookingParams)};

		if (pCooking.get()==nullptr)
		{
			debug_printline(0xffffffff, "Could not create physx Cooking.");
			return false;
		}

		// register callback from physx simulation of contact events.
		pScene->setSimulationEventCallback(&simulationCallback);

		// steal unique ptr's pointers
		physx2::pFoundation = pFoundation.release();
		physx2::pCpuDispatcher = pCPUDispatcher.release();
		physx2::pWorld = pWorld.release();
		physx2::pScene = pScene.release();
		physx2::pCooking = pCooking.release();

		debug_printline(0xffffffff, "Physx successfully created.");

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

		return true;
	}

	/**
	 * \note Declared and called from main.
	 */
	void teardown()
	{
		physx2::pScene->release();

		physx2::pWorld->release();

		PxCloseExtensions();

		physx2::pCpuDispatcher->release();

		physx2::pCooking->release();

		physx2::pFoundation->release();
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

			physx::PxTransform target;
			x.body->getKinematicTarget(target);

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
				make_matrix(core::maths::Quaternionf(pose.q.w, pose.q.x, pose.q.y, pose.q.z))
			};

			engine::graphics::renderer::update(id, std::move(data));
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
