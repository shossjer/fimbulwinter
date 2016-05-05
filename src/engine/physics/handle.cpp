
#include <config.h>

#include <core/debug.hpp>
#include <core/maths/Vector.hpp>

#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Font.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include "PhysxContainer.hpp"

#include <iostream>

#include <engine/hid/input.hpp>

namespace engine
{
namespace physics
{
	class ShapeBox
	{
	public:
		const physx::PxRigidDynamic *const body;
		const physx::PxShape *const shape;

	public:
		ShapeBox(const physx::PxRigidDynamic *const body, const physx::PxShape *const shape)
			: body(body), shape(shape)
		{
		}

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
	};

	class ShapeSphere
	{
		const float r;

		ShapeSphere(const float r)
			: r(r)
		{}
	};

	namespace
	{
		PhysxContainer context{};

		physx::PxScene * scene = nullptr;

		std::vector<physx::PxPlane> planes;
		std::vector<ShapeBox> shapesBoxes;
		std::vector<ShapeSphere> shapeSpheres;
	}

	namespace
	{
		physx::PxController * self = nullptr;
	}
	
	float dx = 0.f;
	float dy = 0.f;
	float dz = 0.f;

	using engine::hid::Input;

	void dispatch(const Input & input)
	{
		switch (input.getState())
		{
		case Input::State::DOWN:
			switch (input.getButton())
			{
			case Input::Button::KEY_ARROWDOWN:
			{
				dz = 0.1f;
				break;
			}
			case Input::Button::KEY_ARROWUP:
			{
				dz = -0.1f;
				break;
			}

			case Input::Button::KEY_ARROWLEFT:
			{
				dx = -0.1f;
				break;
			}
			case Input::Button::KEY_ARROWRIGHT:
			{
				dx = +0.1f;
				break;
			}

			case Input::Button::KEY_SPACEBAR:
			{
				dy = 0.1f;
			//	self->getActor()->addForce(physx::PxVec3(0.f, 200.f, 0.f), physx::PxForceMode::eIMPULSE);
				break;
			}
			default:
				;
			}
			break;
		case Input::State::UP:
			switch (input.getButton())
			{
			case Input::Button::KEY_ARROWDOWN:
			{
				dz = 0.0f;
				break;
			}
			case Input::Button::KEY_ARROWUP:
			{
				dz = 0.0f;
				break;
			}
			case Input::Button::KEY_ARROWLEFT:
			{
				dx = 0.0f;
				break;
			}
			case Input::Button::KEY_ARROWRIGHT:
			{
				dx = 0.0f;
				break;
			}
			default:
				break;
			}
		default:
			;
		}
	}

	const float gScaleFactor = 1.5f;
	const float gStandingSize = 1.0f * gScaleFactor;
	const float gCrouchingSize = 0.25f * gScaleFactor;
	const float gControllerRadius = 0.3f * gScaleFactor;

	void character(const physx::PxExtendedVec3 & pos, physx::PxMaterial * material)
	{
		physx::PxControllerManager *const controllerManager = PxCreateControllerManager(*scene);

		physx::PxCapsuleControllerDesc desc;

		// TODO: add desc
		desc.height = gStandingSize;
		desc.position = pos;
		desc.radius = gControllerRadius;
		desc.material = material;

		if (desc.isValid())
		{
			printf("desc is valid");
		}
		self = controllerManager->createController(desc);
		

		if (self == nullptr)
		{
			context.printError();
			throw std::exception("Could not create self!");
		}
	}

	void setup()
	{
		scene = context.createScene();

		physx::PxMaterial *const material = context.createMaterial();
		physx::PxShape *const shape = context.createShape(physx::PxBoxGeometry(2.f, 2.f, 2.f), material);

		shapesBoxes.reserve(100);
		shapeSpheres.reserve(100);

		planes.reserve(10);

		planes.push_back(physx::PxPlane(physx::PxVec3(0, 1, 0), 0));

		// create the Ground plane
		context.createPlane(planes.back(), material, scene);

		// 
		shapesBoxes.push_back(ShapeBox(
			context.createRigidDynamic(physx::PxTransform(0.f, 20.f, 0.f), shape, scene),
			shape));

		shapesBoxes.push_back(ShapeBox(
			context.createRigidDynamic(physx::PxTransform(0.f, 30.f, 0.f), shape, scene),
			shape));

		shapesBoxes.push_back(ShapeBox(
			context.createRigidDynamic(physx::PxTransform(0.f, 40.f, 0.f), shape, scene),
			shape));

		shapesBoxes.push_back(ShapeBox(
			context.createRigidDynamic(physx::PxTransform(0.f, 50.f, 0.f), shape, scene),
			shape));

		character(physx::PxExtendedVec3(10.f, 10.f, 0.f), material);
	}	

	const float mStepSize = 1.0f / 60.0f;

	void update()
	{
		scene->simulate(mStepSize);
		scene->fetchResults(true);

		const physx::PxVec3 vel = self->getActor()->getLinearVelocity();
		const float deltaY = vel.y*mStepSize - 9.82f*mStepSize*mStepSize*0.5f;

		self->move(physx::PxVec3(dx, dy + deltaY, dz), 0.001f, mStepSize, physx::PxControllerFilters());

		dy = 0.f;
	}

	void draw_box(const physx::PxBoxGeometry & size);

	void render()
	{
		for (const auto & rb : shapesBoxes)
		{
			// TODO: try to get pos updates when objects are moved (and post as pos-event)
			physx::PxTransform pT = physx::PxShapeExt::getGlobalPose(*rb.shape, *rb.body);
			physx::PxBoxGeometry bg;
			rb.shape->getBoxGeometry(bg);

			physx::PxMat33 m = physx::PxMat33(pT.q);
			float mat[16];
			
			ShapeBox::getColumnMajor(m, pT.p, mat);

			if (rb.body->isSleeping())
			{
				glColor3f(1, 0, 0);
			}
			else
			{
				glColor3f(1, 1, 1);
			}

			glPushMatrix();
			glMultMatrixf(mat);

			draw_box(bg);

			glPopMatrix();
		}

		const auto pos = self->getPosition();

		glPushMatrix();
		{
			glTranslated(pos.x, pos.y, pos.z);
			draw_box(physx::PxBoxGeometry(gControllerRadius, gStandingSize, gControllerRadius));
		}
		glPopMatrix();
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
