
#include <config.h>

//   this needs to be first
// vvvvvvvvvvvvvvvvvvvvvvvvvv
#include "PhysxSDK.hpp"
#include "PhysxScene.hpp"
// ^^^^^^^^^^^^^^^^^^^^^^^^^^
//   this needs to be first

#include <core/debug.hpp>
#include <core/maths/Vector.hpp>

#include <engine/physics/physics.hpp>
#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Font.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include <array>
#include <stdexcept>
#include <iostream>

namespace engine
{
namespace physics
{
	const float mStepSize = 1.0f / 60.0f;
	
	void nearby(const PhysxScene & scene, const physx::PxVec3 & pos, const float radie, std::vector<Id> & objects);
		
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

	void nearby(const Point & pos, const float radius, std::vector<Id> & objects)
	{
		// 
		nearby(scene, physx::PxVec3(pos[0], pos[1], pos[2]), radius, objects);
	}

	void update()
	{
		scene.instance->simulate(mStepSize);
		scene.instance->fetchResults(true);
	}

	MoveResult update(const Id id, const MoveData & moveData)
	{
		physx::PxController *const actor = reinterpret_cast<physx::PxController *>(scene.controller(id));

		const float ACC = -9.82f;

		const float dY = moveData.velY*mStepSize + ACC*mStepSize*mStepSize*0.5f;

		physx::PxControllerFilters collisionFilters;

		//physx::PxControllerCollisionFlags collisionFlags
		const physx::PxU32 flags = actor->move(physx::PxVec3(moveData.velXZ[0] * mStepSize * 10.f, dY, moveData.velXZ[1] * mStepSize * 10.f), 0.001f, mStepSize, collisionFilters);

		const physx::PxExtendedVec3 & pos = actor->getPosition();

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

	void create(const Id id, const BoxData & data)
	{
		MaterialData & material = materials.at(data.material);
		physx::PxBoxGeometry geometry(data.size[0], data.size[1], data.size[2]);
		
		physx::PxShape *const shape = context.createShape(geometry, material.pxMaterial);

		const float mass = material.density * (data.size[0] * data.size[1] * data.size[2]) * data.solidity;
		
		physx::PxRigidActor *const actor = context.createRigidDynamic(convert(data.pos), shape, scene.instance, mass);

		//shapesBoxes.back().body->setActorFlag(physx::PxActorFlag::eSEND_SLEEP_NOTIFIES, true);

		scene.actors.emplace(id, actor);
	}

	void create(const Id id, const CharacterData & data)
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
	}

	void remove(const Id id)
	{
		scene.remove(id);
	}

	/**
	 *	Temp Rendering stuff!
	 */

	static void getColumnMajor(physx::PxMat33 m, physx::PxVec3 t, float* mat)
	{
		mat[0] = m.column0[0];
		mat[1] = m.column0[1];
		mat[2] = m.column0[2];
		mat[3] = 0;

		mat[4] = m.column1[0];
		mat[5] = m.column1[1];
		mat[6] = m.column1[2];
		mat[7] = 0;

		mat[8] = m.column2[0];
		mat[9] = m.column2[1];
		mat[10] = m.column2[2];
		mat[11] = 0;

		mat[12] = t[0];
		mat[13] = t[1];
		mat[14] = t[2];
		mat[15] = 1;
	}

	void draw_box(const physx::PxBoxGeometry & size);

	void render()
	{
		for (const auto & rb : scene.actors)
		{
			// TODO: try to get pos updates when objects are moved (and post as pos-event)
		//	physx::PxTransform pT = rb.second->getGlobalPose();//physx::PxShapeExt::getGlobalPose(*rb.second->getShapes()[0], *rb);

			physx::PxShape* shapes[1];
			rb.second->getShapes(shapes, 1);
			
			physx::PxBoxGeometry bg;
			shapes[0]->getBoxGeometry(bg);

			physx::PxTransform pT = rb.second->getGlobalPose();//physx::PxShapeExt::getGlobalPose(*shapes[0], *rb.second);
			

			physx::PxMat33 m = physx::PxMat33(pT.q);
			float mat[16];

			getColumnMajor(m, pT.p, mat);

			//if (rb.second-> ->isSleeping())
			//{
			//	glColor3f(1, 0, 0);
			//}
			//else
			{
				glColor3f(1, 1, 1);
			}

			glPushMatrix();
			glMultMatrixf(mat);

			draw_box(bg);

			glPopMatrix();
		}

		for(const auto & controller : scene.controllers)
		{
			const auto pos = controller.second->getPosition();//self->getPosition();

			physx::PxShape* shapes[1];
			controller.second->getActor()->getShapes(shapes, 1);

			glPushMatrix();
			{
				physx::PxCapsuleGeometry geometry;

				if (shapes[0]->getCapsuleGeometry(geometry))
				{
					glTranslated(pos.x, pos.y, pos.z);
					draw_box(physx::PxBoxGeometry(geometry.radius, 2*geometry.halfHeight, geometry.radius));
					//draw_box(physx::PxBoxGeometry(gControllerRadius, gStandingSize, gControllerRadius));
				}
			}
			glPopMatrix();
		}
	}

	//void draw_plane(const btCollisionShape * const shape)
	//{
	//	const btStaticPlaneShape* staticPlaneShape = static_cast<const btStaticPlaneShape*>(shape);
	//	btScalar planeConst = staticPlaneShape->getPlaneConstant();
	//	const btVector3& planeNormal = staticPlaneShape->getPlaneNormal();
	//	btVector3 planeOrigin = planeNormal * planeConst;
	//	btVector3 vec0, vec1;
	//	btPlaneSpace1(planeNormal, vec0, vec1);
	//	btScalar vecLen = 100.f;
	//	btVector3 pt0 = planeOrigin + vec0*vecLen;
	//	btVector3 pt1 = planeOrigin - vec0*vecLen;
	//	btVector3 pt2 = planeOrigin + vec1*vecLen;
	//	btVector3 pt3 = planeOrigin - vec1*vecLen;
	//	glBegin(GL_LINES);
	//	glVertex3f(pt0.getX(), pt0.getY(), pt0.getZ());
	//	glVertex3f(pt1.getX(), pt1.getY(), pt1.getZ());
	//	glVertex3f(pt2.getX(), pt2.getY(), pt2.getZ());
	//	glVertex3f(pt3.getX(), pt3.getY(), pt3.getZ());
	//	glEnd();
	//}

	//void draw_plane(const physx::PxPlane & plane)
	//{
	//	physx::PxVec3 planeOrigin = plane.n*plane.d;
	//	
	//	btVector3 vec0, vec1;
	//	btPlaneSpace1(planeNormal, vec0, vec1);
	//	btScalar vecLen = 100.f;
	//	btVector3 pt0 = planeOrigin + vec0*vecLen;
	//	btVector3 pt1 = planeOrigin - vec0*vecLen;
	//	btVector3 pt2 = planeOrigin + vec1*vecLen;
	//	btVector3 pt3 = planeOrigin - vec1*vecLen;
	//	glBegin(GL_LINES);
	//	glVertex3f(pt0.getX(), pt0.getY(), pt0.getZ());
	//	glVertex3f(pt1.getX(), pt1.getY(), pt1.getZ());
	//	glVertex3f(pt2.getX(), pt2.getY(), pt2.getZ());
	//	glVertex3f(pt3.getX(), pt3.getY(), pt3.getZ());
	//	glEnd();
	//}

	//void draw_sphere(const btScalar radius, const unsigned int lats, const unsigned int longs)
	//{
	//	int i, j;
	//	for (i = 0; i <= lats; i++) {
	//		btScalar lat0 = SIMD_PI * (-btScalar(0.5) + (btScalar)(i - 1) / lats);
	//		btScalar z0 = radius*sin(lat0);
	//		btScalar zr0 = radius*cos(lat0);

	//		btScalar lat1 = SIMD_PI * (-btScalar(0.5) + (btScalar)i / lats);
	//		btScalar z1 = radius*sin(lat1);
	//		btScalar zr1 = radius*cos(lat1);

	//		glBegin(GL_QUAD_STRIP);
	//		for (j = 0; j <= longs; j++) {
	//			btScalar lng = 2 * SIMD_PI * (btScalar)(j - 1) / longs;
	//			btScalar x = cos(lng);
	//			btScalar y = sin(lng);
	//			glNormal3f(x * zr1, y * zr1, z1);
	//			glVertex3f(x * zr1, y * zr1, z1);
	//			glNormal3f(x * zr0, y * zr0, z0);
	//			glVertex3f(x * zr0, y * zr0, z0);
	//		}
	//		glEnd();
	//	}
	//}

	void draw_box(const physx::PxBoxGeometry & size)
	{
		glScalef(size.halfExtents.x, size.halfExtents.y, size.halfExtents.z);

		glBegin(GL_TRIANGLES);
		// front faces
		glNormal3f(0, 0, 1);
		// face v0-v1-v2
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);
		//glColor3f(1, 1, 0);
		glVertex3f(-1, 1, 1);
		//glColor3f(1, 0, 0);
		glVertex3f(-1, -1, 1);
		// face v2-v3-v0
		//glColor3f(1, 0, 0);
		glVertex3f(-1, -1, 1);
		//glColor3f(1, 0, 1);
		glVertex3f(1, -1, 1);
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);

		// right faces
		glNormal3f(1, 0, 0);
		// face v0-v3-v4
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);
		//glColor3f(1, 0, 1);
		glVertex3f(1, -1, 1);
		//glColor3f(0, 0, 1);
		glVertex3f(1, -1, -1);
		// face v4-v5-v0
		//glColor3f(0, 0, 1);
		glVertex3f(1, -1, -1);
		//glColor3f(0, 1, 1);
		glVertex3f(1, 1, -1);
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);

		// top faces
		glNormal3f(0, 1, 0);
		// face v0-v5-v6
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);
		//glColor3f(0, 1, 1);
		glVertex3f(1, 1, -1);
		//glColor3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		// face v6-v1-v0
		//glColor3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		//glColor3f(1, 1, 0);
		glVertex3f(-1, 1, 1);
		//glColor3f(1, 1, 1);
		glVertex3f(1, 1, 1);

		// left faces
		glNormal3f(-1, 0, 0);
		// face  v1-v6-v7
		//glColor3f(1, 1, 0);
		glVertex3f(-1, 1, 1);
		//glColor3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		//glColor3f(0, 0, 0);
		glVertex3f(-1, -1, -1);
		// face v7-v2-v1
		//glColor3f(0, 0, 0);
		glVertex3f(-1, -1, -1);
		//glColor3f(1, 0, 0);
		glVertex3f(-1, -1, 1);
		//glColor3f(1, 1, 0);
		glVertex3f(-1, 1, 1);

		// bottom faces
		glNormal3f(0, -1, 0);
		// face v7-v4-v3
		//glColor3f(0, 0, 0);
		glVertex3f(-1, -1, -1);
		//glColor3f(0, 0, 1);
		glVertex3f(1, -1, -1);
		//glColor3f(1, 0, 1);
		glVertex3f(1, -1, 1);
		// face v3-v2-v7
		//glColor3f(1, 0, 1);
		glVertex3f(1, -1, 1);
		//glColor3f(1, 0, 0);
		glVertex3f(-1, -1, 1);
		//glColor3f(0, 0, 0);
		glVertex3f(-1, -1, -1);

		// back faces
		glNormal3f(0, 0, -1);
		// face v4-v7-v6
		//glColor3f(0, 0, 1);
		glVertex3f(1, -1, -1);
		//glColor3f(0, 0, 0);
		glVertex3f(-1, -1, -1);
		//glColor3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		// face v6-v5-v4
		//glColor3f(0, 1, 0);
		glVertex3f(-1, 1, -1);
		//glColor3f(0, 1, 1);
		glVertex3f(1, 1, -1);
		//glColor3f(0, 0, 1);
		glVertex3f(1, -1, -1);
		glEnd();
	}
}
}
