
#include <config.h>

//   this needs to be first
// vvvvvvvvvvvvvvvvvvvvvvvvvv
#include "PhysxSDK.hpp"
#include "PhysxScene.hpp"
// ^^^^^^^^^^^^^^^^^^^^^^^^^^
//   this needs to be first

#include "physics.hpp"

#include <core/debug.hpp>
#include <core/maths/Vector.hpp>

#include <engine/graphics/renderer.hpp>

#include <array>
#include <stdexcept>
#include <iostream>

namespace engine
{
namespace physics
{
	const float mStepSize = 1.0f / 60.0f;
	
	void nearby(const PhysxScene & scene, const physx::PxVec3 & pos, const float radie, std::vector<engine::Entity> & objects);
		
	namespace
	{
		PhysxSDK context{};

		PhysxScene scene{ context.createScene() };

		struct MaterialData
		{
			physx::PxMaterial *const pxMaterial;

			const float density;

			MaterialData(physx::PxMaterial *const pxMaterial, const float density)
				:
				pxMaterial(pxMaterial),
				density(density)
			{
			}
		};

		/**
		Water (salt)	1,030
		Plastics	1,175
		Concrete	2,000
		Iron	7, 870
		Lead	11,340

		*/
	//	std::unordered_map<Material, physx::PxMaterial*> materials;
		std::unordered_map<Material, MaterialData> materials;
	}

	void initialize()
	{
		//scene->setSimulationEventCallback(&simulationEventCallback);

		// setup materials
		materials.emplace(Material::MEETBAG, MaterialData(context.createMaterial(.5f, .4f), 1000.f));
		materials.emplace(Material::STONE, MaterialData(context.createMaterial(.4f, .05f), 2000.f));
		materials.emplace(Material::SUPER_RUBBER, MaterialData(context.createMaterial(1.0f, 1.0f), 1200.f));
		materials.emplace(Material::WOOD, MaterialData(context.createMaterial(.35f, .2f), 700.f));

		{
			// create the Ground plane
			context.createPlane(physx::PxPlane(physx::PxVec3(0, 1, 0), 0), materials.at(Material::STONE).pxMaterial, scene.instance);
		}
	}

	void teardown()
	{

	}

	void nearby(const Point & pos, const float radius, std::vector<engine::Entity> & objects)
	{
		// 
		nearby(scene, physx::PxVec3(pos[0], pos[1], pos[2]), radius, objects);
	}

	void update()
	{
		scene.instance->simulate(mStepSize);
		scene.instance->fetchResults(true);

		for (const auto & rb : scene.actors)
		{
			physx::PxTransform pT = rb.second->getGlobalPose();

			physx::PxMat33 m = physx::PxMat33(pT.q);

			engine::graphics::data::ModelviewMatrix modelview;
			modelview.matrix = core::maths::Matrix4x4f{m.column0[0], m.column1[0], m.column2[0], pT.p[0],
			                                           m.column0[1], m.column1[1], m.column2[1], pT.p[1],
			                                           m.column0[2], m.column1[2], m.column2[2], pT.p[2],
			                                           0.f, 0.f, 0.f, 1.f};
			engine::graphics::renderer::update(rb.first, modelview);
		}
	}

	MoveResult update(const engine::Entity id, const MoveData & moveData)
	{
		physx::PxController *const actor = reinterpret_cast<physx::PxController *>(scene.controller(id));

		const float ACC = -9.82f;

		const float dY = moveData.velY*mStepSize + ACC*mStepSize*mStepSize*0.5f;

		physx::PxControllerFilters collisionFilters;

		//physx::PxControllerCollisionFlags collisionFlags
		const physx::PxU32 flags = actor->move(physx::PxVec3(moveData.velXZ[0] * mStepSize * 10.f, dY, moveData.velXZ[1] * mStepSize * 10.f), 0.001f, mStepSize, collisionFilters);

		const physx::PxExtendedVec3 & pos = actor->getPosition();

		engine::graphics::data::ModelviewMatrix modelview;
		modelview.matrix = core::maths::Matrix4x4f{1.f, 0.f, 0.f, float(pos.x),
		                                           0.f, 1.f, 0.f, float(pos.y),
		                                           0.f, 0.f, 1.f, float(pos.z),
		                                           0.f, 0.f, 0.f, 1.f};
		engine::graphics::renderer::update(id, modelview);

		if (flags & physx::PxControllerCollisionFlag::eCOLLISION_DOWN)
		{
			// 
			return MoveResult(true, Point{(float)pos.x, (float)pos.y, (float)pos.z}, 0.f);
		}
		else
		{
			// 
			return MoveResult(false, Point{ (float)pos.x, (float)pos.y, (float)pos.z}, moveData.velY + ACC*mStepSize);
		}
	}

	physx::PxTransform convert(const Point & point)
	{
		return physx::PxTransform(point[0], point[1], point[2]);
	}

	void create(const engine::Entity id, const BoxData & data)
	{
		MaterialData & material = materials.at(data.material);
		physx::PxBoxGeometry geometry(data.size[0], data.size[1], data.size[2]);
		
		physx::PxShape *const shape = context.createShape(geometry, material.pxMaterial);

		const float mass = material.density * (data.size[0] * data.size[1] * data.size[2]) * data.solidity;
		
		physx::PxRigidActor *const actor = context.createRigidDynamic(convert(data.pos), shape, scene.instance, mass);

		//shapesBoxes.back().body->setActorFlag(physx::PxActorFlag::eSEND_SLEEP_NOTIFIES, true);

		scene.actors.emplace(id, actor);

		engine::graphics::data::CuboidC cuboid = {
			core::maths::Matrix4x4f::translation(data.pos[0], data.pos[1], data.pos[2]),
			float(data.size[0]), float(data.size[1]), float(data.size[2]),
			0xffffffff
		};
		engine::graphics::renderer::add(id, cuboid);
	}

	void create(const engine::Entity id, const CharacterData & data)
	{
		MaterialData & material = materials.at(data.material);
		physx::PxCapsuleControllerDesc desc;

		desc.height = data.height;
		desc.position = physx::PxExtendedVec3(data.pos[0], data.pos[1], data.pos[2]);
		desc.radius = data.radius;
		desc.material = material.pxMaterial;
	
		const float mass = material.density * (data.height * data.radius*(data.radius*0.5f)) * data.solidity;
		
		if (!desc.isValid())
		{
			throw std::runtime_error("Controller description is not valid!");
		}

		physx::PxController *const controller = scene.controllerManager->createController(desc);

		if (controller == nullptr)
		{
			throw std::runtime_error("Could not create controller!");
		}

		controller->getActor()->setMass(mass);

		scene.controllers.emplace(id, controller);

		engine::graphics::data::CuboidC cuboid = {
			core::maths::Matrix4x4f::translation(data.pos[0], data.pos[1], data.pos[2]),
			data.radius, data.height, data.radius,
			0xffffffff
		};
		engine::graphics::renderer::add(id, cuboid);
	}

	void remove(const engine::Entity id)
	{
		scene.remove(id);
	}
}
}
