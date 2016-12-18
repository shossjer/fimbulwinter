
#include <config.h>

#if PHYSICS_USE_PHYSX

#include "physics.hpp"

#include <PxPhysicsAPI.h>

#include "actor_physx.hpp"
#include "helper_physx.hpp"
#include "material_physx.hpp"
#include "Callback.hpp"

#include <engine/graphics/renderer.hpp>

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>

#include <unordered_map>

namespace engine
{
namespace physics
{
	constexpr float TIME_STEP = 1.f/60.f;
	constexpr unsigned int ACTORS_MAX = 100;
	constexpr unsigned int ACTORS_GROUP = 20;

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

	namespace
	{
		// Callback instance for engine notifications
		//	* onFalling - called when Actor start falling
		//	* onGrounded - called when Actor is grounded after falling
		const Callback * pCallback;

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

		core::container::CircleQueueSRMW<std::pair<engine::Entity, core::maths::radianf>, 100> queue_headings;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, core::maths::Vector3f>, 100> queue_movements;
	}

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

		if (!sceneDesc.filterShader)
		{
			sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
		}

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
		materials.emplace(Material::MEETBAG, MaterialDef(1000.f, .5f, .4f));
		materials.emplace(Material::STONE, MaterialDef(2000.f, .4f, .05f));
		materials.emplace(Material::SUPER_RUBBER, MaterialDef(1200.f, 1.0f, 1.0f));
		materials.emplace(Material::WOOD, MaterialDef(700.f, .6f, .2f));
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
		const ::core::maths::Vector3f vec;

		actor_mover(const Vector3f vec) : vec(vec) {}

		void operator () (ActorCharacter & x)
		{

		}

		void operator () (ActorDynamic & x)
		{

		}

		void operator () (ActorStatic & x)
		{

		}

		template <typename X>
		void operator () (X & x)
		{
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

		void operator () (ActorDynamic & x)
		{
			x.heading = heading;
		}

		void operator () (ActorStatic & x)
		{
			x.heading = heading;
		}

		template <typename X>
		void operator () (X & x)
		{
			debug_unreachable();
		}
	};

	void update()
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
			std::pair<engine::Entity, core::maths::Vector3f> data;
			while (queue_movements.try_pop(data))
			{
				actors.call(data.first, actor_mover {data.second});
			}
		}

		// Update the physics world
		physx2::pScene->simulate(TIME_STEP);
		physx2::pScene->fetchResults(true);

		// Get movement from all characters
		for (auto && actor:actors.get<ActorCharacter>())
		{
			auto id = engine::Entity {static_cast<engine::Entity::value_type>((std::size_t)actor.body->getUserData())};

			core::maths::Vector3f pos;
			core::maths::Vector3f vel;
			float angle;

			query_position(id, pos, vel, angle);

			core::maths::Vector3f::array_type pb;
			pos.get_aligned(pb);

			core::maths::Vector3f::array_type vb;
			vel.get_aligned(vb);

			engine::graphics::data::ModelviewMatrix data = {
				core::maths::Matrix4x4f::translation(pb[0], pb[1], pb[2]) *
				core::maths::Matrix4x4f::rotation(core::maths::radianf {angle}, 0.f, 0.f, 1.f) *
				core::maths::Matrix4x4f::rotation(actor.heading, 0.f, 1.f, 0.f)
			};

			engine::graphics::renderer::update(id, std::move(data));


			if (actor.debugRenderId!=::engine::Entity::INVALID)
				engine::graphics::renderer::update(actor.debugRenderId, std::move(data));
		}
	}

	/**
	 *	\note Helper method when creating dynamic objects
	 */
	physx::PxRigidDynamic * create(const physx::PxTransform & position, physx::PxShape *const shape, const float mass)
	{
		physx::PxRigidDynamic *const body = physx2::pWorld->createRigidDynamic(position);

		body->attachShape(*shape);

		physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, mass);

		physx2::pScene->addActor(*body);

		return body;
	}

	void create(const engine::Entity id, const BoxData & data)
	{
		const MaterialDef & materialDef = materials.at(data.material);

		physx::PxBoxGeometry geometry(convert<physx::PxVec3>(data.size));

		physx::PxShape *const shape = physx2::pWorld->createShape(geometry, *materialDef.material);

		const float mass = materialDef.density * data.size.volume() * data.solidity;

		physx::PxRigidDynamic *const actor = create(convert<physx::PxTransform>(data.pos), shape, mass);

		actors.emplace<ActorDynamic>(id, ActorDynamic {actor});
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

		actors.emplace<ActorCharacter>(id, ActorCharacter {controller});
	}

	struct remove_actor
	{
		void operator () (const ActorCharacter & x)
		{
			x.body->release();
		}

		void operator () (const ActorDynamic & x)
		{
			x.body->release();
		}

		void operator () (const ActorStatic & x)
		{
			x.body->release();
		}

		template <typename X>
		void operator () (const X & x)
		{
			debug_unreachable();
		}
	};

	void remove(const engine::Entity id)
	{
		actors.call(id, remove_actor {});

		actors.remove(id);

		//// remove debug object from Renderer
		//if (actor.debugRenderId!= ::engine::Entity::INVALID)
		//	engine::graphics::renderer::remove(actor.debugRenderId);
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
		core::maths::Vector3f velocity;
		float angle;

		void operator () (const ActorCharacter & x)
		{
			const auto body = x.body;

			this->position = convert(body->getPosition());
			this->velocity = convert(body->getActor()->getLinearVelocity());
			this->angle = 0.f;// body->getActor()->get;
		}

		void operator () (const ActorDynamic & x)
		{
			const auto body = x.body;

			this->position = convert(body->getGlobalPose());
			this->velocity = convert(body->getLinearVelocity());
			this->angle = 0.f;//body->GetAngle();
		}

		void operator () (const ActorStatic & x)
		{
		}

		template <typename X>
		void operator () (const X & x)
		{
			debug_unreachable();
		}
	};

	void query_position(const engine::Entity id, Vector3f & position, Vector3f & velocity, float & angle)
	{
		get_transformation data {};

		actors.call(id, data);

		position = data.position;
		velocity = data.velocity;
		angle = data.angle;
	}

	void post_movement(engine::Entity id, core::maths::Vector3f movement)
	{
		const auto res = queue_movements.try_emplace(id, movement);
		debug_assert(res);
	}
	void post_heading(engine::Entity id, core::maths::radianf rotation)
	{
		const auto res = queue_headings.try_emplace(id, rotation);
		debug_assert(res);
	}
}
}

#endif