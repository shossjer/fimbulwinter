
#include <config.h>

#if PHYSICS_USE_PHYSX

#include <PxPhysicsAPI.h>

#include "physics.hpp"

#include "actor_physx.hpp"
#include "filter_physx.hpp"
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
	using namespace physx;

	namespace
	{
		core::container::CircleQueueSRMW<std::pair<engine::Entity, ActorData>, 100> queue_create;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, PlaneData>, 100> queue_create_planes;
		core::container::CircleQueueSRMW<engine::Entity, 100> queue_remove;
	}

	struct reply_shapes
	{
		float totalMass;
		std::vector<physx::PxShape *> shapes;
	};

	/**
		\note create shapes for the Actor body.
		\return float total mass of all shapes
	 */
	reply_shapes create(const std::vector<ShapeData> shapeDatas, PxFilterData filterData)
	{
		reply_shapes reply;
		reply.totalMass = 0.f;

		for (const ShapeData & shapeData : shapeDatas)
		{
			const MaterialDef & materialDef = materials.at(shapeData.material);

			physx::PxShape * shape;

			switch (shapeData.type)
			{
				// TODO: set transform of the shape

				case ShapeData::Type::BOX:
				{
					shape = physx2::pWorld->createShape(
						PxBoxGeometry {physx::PxVec3 {shapeData.geometry.box.w, shapeData.geometry.box.h, shapeData.geometry.box.d}},
						*materialDef.material);

					// calculate mass of the shape
					reply.totalMass += (materialDef.density * shapeData.geometry.box.volume() * shapeData.solidity);
					break;
				}
				case ShapeData::Type::SPHERE:
				{
					shape = physx2::pWorld->createShape(
						PxSphereGeometry {shapeData.geometry.sphere.r},
						*materialDef.material);

					// calculate mass of the shape
					reply.totalMass += (materialDef.density * shapeData.geometry.sphere.volume() * shapeData.solidity);
					break;
				}
				case ShapeData::Type::MESH:
				{
					physx::PxConvexMeshDesc desc;

					desc.points.data = shapeData.geometry.mesh.points.data();
					desc.points.count = shapeData.geometry.mesh.size();
					desc.points.stride = 3*sizeof(float);

					desc.flags = PxConvexFlag::eCOMPUTE_CONVEX;

					PxDefaultMemoryOutputStream buf;
					PxConvexMeshCookingResult::Enum result;

					if (!desc.isValid())
					{
						throw std::runtime_error("Could not create mesh from complicated points.");
					}

					// cook the points into a nice mesh
					if (!physx2::pCooking->cookConvexMesh(desc, buf, &result))
					{
						// possible to use more complicated method to calculate mesh...
						throw std::runtime_error("Could not create mesh from complicated points.");
					}

					PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
					PxConvexMesh* convexMesh = physx2::pWorld->createConvexMesh(input);

					shape = physx2::pWorld->createShape(PxConvexMeshGeometry(convexMesh), *materialDef.material);

					// get the mass properties of the mesh shape
					{
						PxReal mass;
						PxMat33 inertia;
						PxVec3 centre;

						convexMesh->getMassInformation(mass, inertia, centre);
						reply.totalMass += (mass * shapeData.solidity);
					}
					break;
				}
				case ShapeData::Type::CAPSULE:
				default:

				debug_unreachable();
			}

			// save material enum id in filter
			filterData.word2 = static_cast<physx::PxU32>(shapeData.material);

			// apply delta tranform of shape relative to actors centre
			shape->setLocalPose(PxTransform(convert<PxVec3>(shapeData.pos), convert(shapeData.rot)));

			// set filtering data for the shape
			shape->setSimulationFilterData(filterData);

			reply.shapes.push_back(shape);
		}

		return reply;
	}

	/**
		\note Create Filter for Kinematic Actor based on behaviour
	*/
	PxFilterData createFilter(const ActorData::Behaviour behaviour)
	{
		const PxU32 flag = static_cast<PxU32>(behaviour);
		PxU32 mask;

		switch (behaviour)
		{
			case ActorData::Behaviour::TRIGGER:
			{
				mask =	static_cast<PxU32>(ActorData::Behaviour::PLAYER)|
						//static_cast<PxU32>(ActorData::Behaviour::OBSTACLE)|
						static_cast<PxU32>(ActorData::Behaviour::DEFAULT);
				break;
			}
			// obstacles should have full callback (temp disabled)
			case ActorData::Behaviour::OBSTACLE:
			{
				mask = 0;
				/*static_cast<PxU32>(Flag::DYNAMIC)|
				static_cast<PxU32>(Flag::PLAYER)|
				static_cast<PxU32>(Flag::OBSTACLE)|
				static_cast<PxU32>(Flag::STATIC);*/
				break;
			}
			// players should have full callback
			case ActorData::Behaviour::PLAYER:
			{
				mask =	static_cast<PxU32>(ActorData::Behaviour::PLAYER)|
						static_cast<PxU32>(ActorData::Behaviour::OBSTACLE)|
						static_cast<PxU32>(ActorData::Behaviour::DEFAULT)|
						static_cast<PxU32>(ActorData::Behaviour::PROJECTILE);
				break;
			}
			// no callback is needed for dynamic default objects
			case ActorData::Behaviour::DEFAULT:
			{
				mask =	static_cast<PxU32>(0);
				break;
			}
			case ActorData::Behaviour::PROJECTILE:
			{
				mask =	static_cast<PxU32>(ActorData::Behaviour::PLAYER)|
						static_cast<PxU32>(ActorData::Behaviour::OBSTACLE)|
						static_cast<PxU32>(ActorData::Behaviour::DEFAULT);
				break;
			}
			default:
			// Behaviour is not valid for Actor type
			debug_unreachable();
		}

		return PxFilterData {flag, mask, PxU32 {0}, PxU32 {0}};
	}

	/**
		\note Create Actor based on ActorData
	 */
	void create(const engine::Entity id, const ActorData & data)
	{
		switch (data.type)
		{
			case ActorData::Type::DYNAMIC:
			{
				// create a dynamic body at position
				physx::PxRigidDynamic *const body = physx2::pWorld->createRigidDynamic(PxTransform{convert<physx::PxVec3>(data.pos), convert(data.rot)});

				if (data.behaviour==ActorData::Behaviour::PLAYER)
				{
					// setting more powerful damping if player
					body->setAngularDamping(0.5f);
					body->setLinearDamping(2.f);
				}

				// create collision filter flags
				PxFilterData filterData = createFilter(data.behaviour);

				// create shapes
				reply_shapes reply = create(data.shapes, filterData);

				// make the shapes trigger shapes.
				for (auto shape:reply.shapes)
				{
					body->attachShape(*shape);
				}

				// update mass for Actor for non-static body
				physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, reply.totalMass);

				// register it as an actor so we have access to the body from id
				actors.emplace<ActorDynamic>(id, body);

				// set entity id to the body for callback
				body->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				// finally add it to the scene
				physx2::pScene->addActor(*body);
				break;
			}
			case ActorData::Type::PROJECTILE:
			{
				// create a dynamic body at position
				physx::PxRigidDynamic *const body = physx2::pWorld->createRigidDynamic(
					PxTransform {convert<physx::PxVec3>(data.pos), convert(data.rot)});

				// create collision filter flags
				PxFilterData filterData = createFilter(data.behaviour);

				reply_shapes reply = create(data.shapes, filterData);

				for (auto shape:reply.shapes)
				{
					body->attachShape(*shape);
				}

				// update mass for Actor for non-static body
				physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, reply.totalMass);

				// register it as an actor so we have access to the body from id
				actors.emplace<ActorDynamic>(id, body);

				// set entity id to the body for callback
				body->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				physx2::pScene->addActor(*body);
				break;
			}
			case ActorData::Type::KINEMATIC:
			{
				// create a dynamic body at position
				physx::PxRigidDynamic *const body = physx2::pWorld->createRigidDynamic(PxTransform{convert<physx::PxVec3>(data.pos), convert(data.rot)});

				// make it an kinematic object
				body->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, true);

				// create collision filter flags
				PxFilterData filterData = createFilter(data.behaviour);

				// create shapes
				reply_shapes reply = create(data.shapes, filterData);

				// make the shapes trigger shapes.
				for (auto shape : reply.shapes)
				{
					body->attachShape(*shape);
				}

				// update mass for Actor for non-static body
				physx::PxRigidBodyExt::setMassAndUpdateInertia(*body, reply.totalMass);

				// register it as an actor so we have access to the body from id
				actors.emplace<ActorDynamic>(id, body);

				// set entity id to the body for callback
				body->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				// finally add it to the scene
				physx2::pScene->addActor(*body);
				break;
			}
			case ActorData::Type::STATIC:
			{
				// create a dynamic body at position
				physx::PxRigidStatic *const body = physx2::pWorld->createRigidStatic(PxTransform{convert<physx::PxVec3>(data.pos), convert(data.rot)});

				// create collision filter flags
				PxFilterData filterData = createFilter(data.behaviour);

				// create shapes
				reply_shapes reply = create(data.shapes, filterData);

				// make the shapes trigger shapes.
				for (auto shape : reply.shapes)
				{
					body->attachShape(*shape);
				}

				// register it as an actor so we have access to the body from id
				actors.emplace<ActorStatic>(id, body);

				// set entity id to the body for callback
				body->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				// finally add it to the scene
				physx2::pScene->addActor(*body);
				break;
			}
			case ActorData::Type::TRIGGER:
			{
				// create a dynamic body at position
				physx::PxRigidStatic *const body = physx2::pWorld->createRigidStatic(PxTransform{convert<physx::PxVec3>(data.pos), convert(data.rot)});

				// create collision filter flags
				PxFilterData filterData = createFilter(data.behaviour);

				// create shapes
				reply_shapes reply = create(data.shapes, filterData);

				// make the shapes trigger shapes.
				for (auto shape : reply.shapes)
				{
					shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
					shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);

					body->attachShape(*shape);
				}

				// register it as an actor so we have access to the body from id
				actors.emplace<ActorStatic>(id, body);

				// set entity id to the body for callback
				body->userData = (void*) (std::size_t)static_cast<engine::Entity::value_type>(id);

				// finally add it to the scene
				physx2::pScene->addActor(*body);

				break;
			}
		}
	}

	void create(const engine::Entity id, const PlaneData & data)
	{
		const MaterialDef & materialDef = materials.at(data.material);

		physx::PxPlane plane {
			physx::PxVec3 {convert<physx::PxVec3>(data.point)},
			physx::PxVec3 {convert<physx::PxVec3>(data.normal)}};

		physx2::pScene->addActor(*physx::PxCreatePlane(*physx2::pWorld, plane, *materialDef.material));
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
			}
		}

		// poll planes to create
		{
			std::pair<engine::Entity, PlaneData> data;
			while (queue_create_planes.try_pop(data))
			{
				create(data.first, data.second);
			}
		}

		// poll boxes to create
		{
			std::pair<engine::Entity, ActorData> data;
			while (queue_create.try_pop(data))
			{
				create(data.first, data.second);
			}
		}
	}

	void post_create(const engine::Entity id, const ActorData & data)
	{
		const auto res = queue_create.try_push(std::pair<engine::Entity, ActorData>(id, data));
		debug_assert(res);
	}

	void post_create(const engine::Entity id, const PlaneData & data)
	{
		const auto res = queue_create_planes.try_push(std::pair<engine::Entity, PlaneData>(id, data));
		debug_assert(res);
	}

	void post_remove(const engine::Entity id)
	{
		const auto res = queue_remove.try_push(id);
		debug_assert(res);
	}
}
}

#endif
