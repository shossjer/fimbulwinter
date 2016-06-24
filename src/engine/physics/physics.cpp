
#include <config.h>

#include "physics.hpp"

#include <Box2D/Box2D.h>

#include <core/maths/Vector.hpp>

#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Font.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

//#include <core/debug.hpp>

#include <array>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

namespace engine
{
namespace physics
{
	const float timeStep = 1.f / 60.f;

	b2Vec2 convert(const Point & point) { return b2Vec2{ point[0], point[1] }; }
	Point convert(const b2Vec2 & point) { return Point{ point.x, point.y, 0.f }; }
	/**
	 *	Contact counter for characters to determine falling / grounded
	 */
	struct ShapeContactCounter
	{
	private:
		unsigned int contacts;

	public:
		b2Fixture *const fixture;

	public:
		ShapeContactCounter(b2Fixture *const fixture)
			:
			contacts{ 0 },
			fixture{ fixture }
		{}

		bool isGrounded() const { return this->contacts != 0; }

		void increment() { this->contacts++; }
		void decrement() { this->contacts--; }
	};
	/**
	 *	Contact change Callback
	 */
	class MyContactListener : public b2ContactListener
	{
		void BeginContact(b2Contact* contact)
		{
			if (contact->GetFixtureA()->GetUserData())
			{
				reinterpret_cast<ShapeContactCounter*>(contact->GetFixtureA()->GetUserData())->increment();
			}

			if (contact->GetFixtureB()->GetUserData())
			{
				reinterpret_cast<ShapeContactCounter*>(contact->GetFixtureB()->GetUserData())->increment();
			}
		}

		void EndContact(b2Contact* contact)
		{
			if (contact->GetFixtureA()->GetUserData())
			{
				reinterpret_cast<ShapeContactCounter*>(contact->GetFixtureA()->GetUserData())->decrement();
			}

			if (contact->GetFixtureB()->GetUserData())
			{
				reinterpret_cast<ShapeContactCounter*>(contact->GetFixtureB()->GetUserData())->decrement();
			}
		}
	}	contactCallback;
		
	namespace
	{
		/**
		 *	The World
		 */
		b2World world{ b2Vec2{0.f, -9.82f} };
		/**
		 *	Helper function to also set id!
		 */
		b2Body * createBody(const engine::Entity id, b2BodyDef & def)
		{
			b2Body * body = world.CreateBody(&def);

			body->SetUserData((void*)static_cast<engine::Entity::value_type>(id));

			return body;
		}
		/**
		 *	Material properties, used when creating actors 
		 */
		struct MaterialData
		{
			const float density;
			const float friction;
			const float restitution;

			MaterialData(const float density, const float friction, const float restitution)
				:
				density(density),
				friction(friction),
				restitution(restitution)
			{
			}
		};

		std::unordered_map<engine::Entity, b2Body*> actors;
		std::unordered_map<engine::Entity, ShapeContactCounter> characterContactCounters;

		std::unordered_map<Material, MaterialData> materials;
	}

	void initialize()
	{
		world.SetContactListener(&contactCallback);
		{
			const engine::Entity id{ 0 };
			// This a chain shape with isolated vertices
			std::vector<b2Vec2> vertices;
		
			vertices.push_back(b2Vec2(30.f, 0.f));
			vertices.push_back(b2Vec2(15.f, 0.f));
			vertices.push_back(b2Vec2(15.f, 10.f));
			vertices.push_back(b2Vec2(10.f, 10.f));
			vertices.push_back(b2Vec2(10.f, 0.f));
			vertices.push_back(b2Vec2(0.f, 0.f));
			vertices.push_back(b2Vec2(-10.f, 10.f));
			vertices.push_back(b2Vec2(-10.f, 0.f));
			b2ChainShape chain;
			chain.CreateChain(vertices.data(), vertices.size());

			b2BodyDef bodyDef;
			bodyDef.type = b2_staticBody;
			bodyDef.position.Set(0.f, 0.f);
			b2Body* body = createBody(id, bodyDef);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &chain;
			fixtureDef.density = 1.f;
			fixtureDef.friction = .3f;

			body->CreateFixture(&fixtureDef);

			actors.emplace(id, body);
		}
		/**
			Water (salt)	1,030
			Plastics	1,175
			Concrete	2,000
			Iron	7, 870
			Lead	11,340
		 */
		// setup materials
		materials.emplace(Material::MEETBAG, MaterialData(1000.f, .5f, .4f));
		materials.emplace(Material::STONE, MaterialData(2000.f, .4f, .05f));
		materials.emplace(Material::SUPER_RUBBER, MaterialData(1200.f, 1.0f, 1.0f));
		materials.emplace(Material::WOOD, MaterialData(700.f, .6f, .2f));
	}

	void teardown()
	{

	}

	void nearby(const Point & pos, const float radius, std::vector<engine::Entity> & objects)
	{
		// 
		class AABBQuery : public b2QueryCallback
		{
			std::vector<engine::Entity> & objects;
		public:
			AABBQuery(std::vector<engine::Entity> & objects) : objects(objects)	{}

			bool ReportFixture(b2Fixture* fixture)
			{
				const b2Body *const body = fixture->GetBody();

				objects.push_back(engine::Entity{ reinterpret_cast<engine::Entity::value_type>(body->GetUserData()) });

				// Return true to continue the query.
				return true;
			}

		} query{ objects };

		b2AABB aabb{};

		aabb.lowerBound.Set(pos[0] - radius, pos[1] - radius);
		aabb.upperBound.Set(pos[0] + radius, pos[1] + radius);

		world.QueryAABB(&query, aabb);
	}

	//class RayCast
	//{
	//public:
	//	virtual float callback(const Id id, const Point & point, const Point & normal, const float fraction) = 0;

	//	void rayCast(const Point & from, const Point & to);
	//};

	//void RayCast::execute(const Point & from, const Point & to)
	//{
	//	// This class captures the closest hit shape.
	//	class MyRayCastCallback : public b2RayCastCallback
	//	{
	//		RayCast & reporter;
	//	public:
	//		MyRayCastCallback(RayCast & reporter) : reporter(reporter) {}

	//		float32 ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float32 fraction)
	//		{
	//			return reporter.callback((Id)fixture->GetBody()->GetUserData(), convert(point), convert(normal), fraction);
	//		}
	//	} query{ *this };

	//	world.RayCast(&query, convert(from), convert(to));
	//}

	void update()
	{
		const int32 velocityIterations = 6;
		const int32 positionIterations = 2;

		world.Step(timeStep, velocityIterations, positionIterations);
	}

	MoveResult update(const engine::Entity id, const MoveData & moveData)
	{
		b2Body *const body = actors.at(id);

		const float ACC = -9.82f;
		const float velY = moveData.velY + ACC*timeStep;

		body->SetLinearVelocity(b2Vec2(moveData.velXZ[0] * 4.f, velY));

		const b2Vec2 & pos = body->GetPosition();

		if (characterContactCounters.at(id).isGrounded())
		{
			// I check the velY < 0.f since otherwise jumps are interrupted. Contact updating is too slow.
			if (velY < 0.f)
			{
				return MoveResult(true, Point{ (float)pos.x, (float)pos.y, (float)0.f }, 0.f);
			}
		}

		// 
		return MoveResult(false, Point{ (float)pos.x, (float)pos.y, (float)0.f}, velY);
	}

	void create(const engine::Entity id, const BoxData & data)
	{
		const MaterialData & material = materials.at(data.material);

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(data.pos[0], data.pos[1]);
		
		b2Body*const body = createBody(id, bodyDef);

		b2PolygonShape dynamicBox;
		dynamicBox.SetAsBox(data.size[0], data.size[1]);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &dynamicBox;
		fixtureDef.density = material.density;
		fixtureDef.friction = material.friction;
		fixtureDef.restitution = material.restitution;

		b2Fixture *const fixture = body->CreateFixture(&fixtureDef);

		actors.emplace(id, body);
	}

	void create(const engine::Entity id, const CharacterData & data)
	{
		const MaterialData & material = materials.at(data.material);
		//	debug_assert(data.height > (2 * data.radius));

		const float halfRadius = data.radius*.5f;
		const float halfHeight = data.height*.5f;

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.position.Set(data.pos[0], data.pos[1]);
		
		b2Body*const body = createBody(id, bodyDef);
		{
			b2PolygonShape shape;
			shape.SetAsBox(halfRadius, halfHeight - halfRadius);

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shape;
			fixtureDef.density = material.density;
			fixtureDef.friction = material.friction;
			fixtureDef.restitution = material.restitution;

			body->CreateFixture(&fixtureDef);
		}
		{
			// upper sphere of the "capsule"
			b2CircleShape shape;
			shape.m_radius = halfRadius;
			shape.m_p = b2Vec2(0.f, (halfHeight - halfRadius));	// adjust position

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shape;
			fixtureDef.density = material.density;
			fixtureDef.friction = material.friction;
			fixtureDef.restitution = material.restitution;

			body->CreateFixture(&fixtureDef);
		}
		{
			// lower sphere of the "capsule"
			b2CircleShape shape;
			shape.m_radius = halfRadius;
			shape.m_p = b2Vec2(0.f, (-halfHeight + halfRadius));	// adjust position

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shape;
			fixtureDef.density = material.density;
			fixtureDef.friction = 1.f;//material.friction;
			fixtureDef.restitution = 1.f;//material.restitution;
			//fixtureDef.isSensor = true;	// lower spape keeps track of contacts
			b2Fixture *const feets = body->CreateFixture(&fixtureDef);

			characterContactCounters.emplace(id, ShapeContactCounter{ feets });

			feets->SetUserData(&characterContactCounters.at(id));
		}

		body->SetFixedRotation(true);

		actors.emplace(id, body);
	}

	void remove(const engine::Entity id)
	{
		actors.erase(id);
		characterContactCounters.erase(id);

		//auto t = characterContactCounters.find(id);

		//if (t != characterContactCounters.end())
		//{
		//	characterContactCounters.erase(id);
		//}
	}

	/**
	 *	Temp Rendering stuff!
	 */

	const float DEGTORAD = 3.14f / 180.f;
	const float RADTODEG = 180.f / 3.14f;

	void draw_debug(const b2Body *const body)
	{
		const b2Transform & transform = body->GetTransform();

		glPushMatrix();
		{
			glTranslatef(transform.p.x, transform.p.y, 0.f);
		//	glScalef(scale.x, scale.y, scale.x);
			glRotatef(transform.q.GetAngle()*RADTODEG, 0.f, 0.f, 1.f);

			for (const b2Fixture *f = body->GetFixtureList(); f != nullptr; f = f->GetNext())
			{
				switch (f->GetType())
				{
				case b2Shape::Type::e_circle:
					{
						const b2CircleShape *const shape = reinterpret_cast<const b2CircleShape *>(f->GetShape());

						glPushMatrix();
						{
							glTranslatef(shape->m_p.x, shape->m_p.y, 0.f);
							glScalef(shape->m_radius, shape->m_radius, 1.f);

							glBegin(GL_LINE_STRIP);
							{
								for (float a = 0; a < 360 * DEGTORAD; a += 30 * DEGTORAD)
								{
									glVertex2f(sinf(a), cosf(a));
								}

								glVertex2f(0.f, 1.f); // last point on top
								glVertex2f(0.f, 0.f); // to centre
								glVertex2f(1.f, 0.f); // and right
							}
							glEnd();

						}
						glPopMatrix();
					}
					break;

				case b2Shape::Type::e_polygon:
					{
						const b2PolygonShape *const shape = reinterpret_cast<const b2PolygonShape *>(f->GetShape());

						glBegin(GL_LINE_LOOP);
						{
							for (int i = 0; i < shape->GetVertexCount(); i++)
							{
								const b2Vec2 & point = shape->GetVertex(i);

								glVertex2f(point.x, point.y);
							}
						}
						glEnd();
					}
					break;

				case b2Shape::Type::e_chain:
					{
						const b2ChainShape *const shape = reinterpret_cast<const b2ChainShape *>(f->GetShape());
					
						glPushMatrix();
						{
							glTranslatef(transform.p.x, transform.p.y, 0.f);
							//	glScalef(0.5f, 0.5f, 0.5f);
							glRotatef(transform.q.GetAngle(), 0.f, 0.f, 1.f);

							glBegin(GL_LINE_STRIP);
							{
								for (int32 i = 0; i < shape->m_count; i++)
								{
									const b2Vec2 point = *(shape->m_vertices + i);

									glVertex3f(point.x, point.y, 0.f);
								}
							}
							glEnd();
						}
						glPopMatrix();
					}
					break;

				default:
					break;
				}
			}
		}
		glPopMatrix();
	}

	void render()
	{
		glColor3ub(255, 255, 255);

		for (const auto & item : actors)
		{
			draw_debug(item.second);
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

	void draw_chain(const b2Transform & transform, const b2Vec2 *const vertices, const int32 verticesNum)
	{
		glPushMatrix();
		{
			glTranslatef(transform.p.x, transform.p.y, 0.f);
		//	glScalef(0.5f, 0.5f, 0.5f);
			glRotatef(transform.q.GetAngle(), 0.f, 0.f, 1.f);

			glBegin(GL_LINE_STRIP);
			{
				for (int32 i = 0; i < verticesNum; i++)
				{
					const b2Vec2 point = *(vertices + i);
					glVertex3f(point.x, point.y, 0.f);
				}
			}
			glEnd();
		}
		glPopMatrix();
	}

	void draw_box(const b2Transform & transform, const b2Vec2 & scale)
	{
		glPushMatrix();
		{
			glTranslatef(transform.p.x, transform.p.y, 0.f);
			glScalef(scale.x, scale.y, scale.x);
			glRotatef(transform.q.GetAngle()*180.f/ 3.14159f, 0.f, 0.f, 1.f);

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
		glPopMatrix();
	}
}
}
