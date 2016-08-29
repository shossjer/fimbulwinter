
#include <config.h>

#include "Callbacks.hpp"
#include "helper.hpp"
#include "physics.hpp"
#include "queries.hpp"

#include <Box2D/Box2D.h>

#include <core/container/Collection.hpp>
#include <core/maths/Vector.hpp>

#include <engine/graphics/renderer.hpp>

#include <array>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

namespace engine
{
namespace physics
{
	const float timeStep = 1.f / 60.f;

	b2Vec2 convert(const core::maths::Vector3f val)
	{
		core::maths::Vector3f::array_type buffer;
		val.get_aligned(buffer);
		return b2Vec2{ buffer[0], buffer[1] };
	}
	core::maths::Vector3f convert(const b2Vec2 val)
	{
		return core::maths::Vector3f{ val.x, val.y, 0.f };
	}
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
	
		
	namespace
	{
		const Callbacks * pCallbacks;
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

			body->SetUserData((void*)(std::size_t)static_cast<engine::Entity::value_type>(id));

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

		struct PhysicsActor
		{
			b2Body *const body;
			::engine::Entity debugRenderId;

			PhysicsActor(b2Body *const body, const ::engine::Entity debugRenderId)
				:
				body(body),
				debugRenderId(debugRenderId)
			{}
		};

	//	std::unordered_map<engine::Entity, b2Body*> actors;
		std::unordered_map<engine::Entity, PhysicsActor> actors;
		std::unordered_map<engine::Entity, ShapeContactCounter> contactCounter;

		std::unordered_map<Material, MaterialData> materials;

		// We need more flexibility than  just the one actors array in
		// order to send position data only to the entities that needs
		// it, else it  is just wasteful.  These  example arrays store
		// the entities used for boundingbox drawings. -- shossjer

		// struct debug_immovable_t  // things  that do  not
		// move { engine::Entity debug_id;

		// 	debug_immovable_t(engine::Entity debug_id) : debug_id(debug_id) {}
		// };
		// struct debug_movable_t // things that move
		// {
		// 	engine::Entity debug_id;

		// 	debug_movable_t(engine::Entity debug_id) : debug_id(debug_id) {}
		// };

		// core::container::Collection
		// <
		// 	engine::Entity,
		// 	1001,
		// 	std::array<debug_immovable_t, 100>,
		// 	std::array<debug_movable_t, 400>
		// >
		// debug_entities;
	}

	/**
	 *	Contact change Callback
	 */
	class MyContactListener : public b2ContactListener
	{
	private:

		void BeginContact(const b2Fixture *const fixA, const b2Fixture *const fixB, const b2Contact *const contact)
		{
			auto id = engine::Entity{ static_cast<engine::Entity::value_type>((std::size_t)fixA->GetUserData()) };

			auto itr = contactCounter.find(id);

			if (itr != contactCounter.end())
			{
				itr->second.increment();

				// TODO: calculate ground normal if more then one contact
				pCallbacks->onGrounded(id, convert(contact->GetManifold()->localNormal));
			}
		}

		void EndContact(const b2Fixture *const fixA, const b2Fixture *const fixB, const b2Contact *const contact)
		{
			auto id = engine::Entity{ static_cast<engine::Entity::value_type>((std::size_t)fixA->GetUserData()) };

			auto itr = contactCounter.find(id);

			if (itr != contactCounter.end())
			{
				itr->second.decrement();

				if (itr->second.isGrounded())
				{
					// TODO: calculate ground normal if more then one contact
					pCallbacks->onGrounded(id, convert(contact->GetManifold()->localNormal));
				}
				else
				{
					pCallbacks->onFalling(id);
				}
			}
		}

	public:

		void BeginContact(b2Contact* contact)
		{
			if (contact->GetFixtureA()->GetUserData())
			{
				BeginContact(contact->GetFixtureA(), contact->GetFixtureB(), contact);
			}

			if (contact->GetFixtureB()->GetUserData())
			{
				BeginContact(contact->GetFixtureB(), contact->GetFixtureA(), contact);
			}

			printf("contact type: %d, normal: %f, %f, points: %d (grounded)\n", contact->GetManifold()->type, contact->GetManifold()->localNormal.x, contact->GetManifold()->localNormal.y, contact->GetManifold()->pointCount);
		}

		void EndContact(b2Contact* contact)
		{
			if (contact->GetFixtureA()->GetUserData())
			{
				EndContact(contact->GetFixtureA(), contact->GetFixtureB(), contact);
			}

			if (contact->GetFixtureB()->GetUserData())
			{
				EndContact(contact->GetFixtureB(), contact->GetFixtureA(), contact);
			}

			printf("contact type: %d, normal: %f, %f, points: %d (falling)\n", contact->GetManifold()->type, contact->GetManifold()->localNormal.x, contact->GetManifold()->localNormal.y, contact->GetManifold()->pointCount);
		}

	}	contactCallback;

	namespace query
	{
		::core::maths::Vector3f positionOf(const engine::Entity id)
		{
			const b2Vec2 point = actors.at(id).body->GetPosition();

			return ::core::maths::Vector3f{ point.x, point.y, 0.f };
		}

		void positionOf(const engine::Entity id, Point & pos, Vector & velocity)
		{
			const b2Vec2 point = actors.at(id).body->GetPosition();
			const b2Vec2 vel = actors.at(id).body->GetLinearVelocity();

			pos[0] = point.x;
			pos[1] = point.y;
			pos[2] = 0.f;

			velocity[0] = vel.x;
			velocity[1] = vel.y;
			velocity[2] = 0.f;
		}

		std::vector<Actor> load(const std::vector<engine::Entity> & targets)
		{
			std::vector<Actor> reply;
			reply.reserve(targets.size());

			for (const auto val : targets)
			{
				reply.emplace_back(Actor{ val, actors.at(val).body } );
			}

			return reply;
		}

		Actor load(const engine::Entity id)
		{
			return Actor{ id, actors.at(id).body };
		}
	}

	const b2World & getWorld()
	{
		return world;
	}

	void initialize(const Callbacks & callbacks)
	{
		pCallbacks = &callbacks;
	
		world.SetContactListener(&contactCallback);
		{

			const engine::Entity id = engine::Entity::create();
			// const auto id = engine::Entity::create(); // does not work :/
			// the above yields the following runtime error from somewhere at startup:
			// > terminate called after throwing an instance of 'std::out_of_range'
			// >   what():  _Map_base::at
			// > Aborted

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

			// actors.emplace(id, body); // this was needed before when we drew the actors from this file, but we do not anymore

			// debug graphics
			{
				const auto debug_id = engine::Entity::create(); // add this one line and the program throws an exception somewhere else at some other time, yay!
				// debug_entities.add(id, debug_immovable_t{debug_id});

				core::container::Buffer vertices_;
				vertices_.resize<float>(3 * vertices.size());
				for (std::size_t i = 0; i < vertices.size(); i++)
				{
					vertices_.data_as<float>()[i * 3 + 0] = vertices[i].x;
					vertices_.data_as<float>()[i * 3 + 1] = vertices[i].y;
					vertices_.data_as<float>()[i * 3 + 2] = 0.f;
				}
				core::container::Buffer edges_;
				edges_.resize<uint16_t>(2 * (vertices.size() - 1));
				for (std::size_t i = 0; i < vertices.size() - 1; i++)
				{
					edges_.data_as<uint16_t>()[i * 2 + 0] = (uint16_t)i;
					edges_.data_as<uint16_t>()[i * 2 + 1] = (uint16_t)i + 1;
				}

				engine::graphics::data::LineC data = {
					core::maths::Matrix4x4f::identity(),
					std::move(vertices_),
					std::move(edges_),
					0xffffffff
				};
				engine::graphics::renderer::add(id, std::move(data));
				// engine::graphics::renderer::add(debug_id, std::move(data));
			}
		}
		/**
			Water (salt)	1,030
			Plastics		1,175
			Concrete		2,000
			Iron			7, 870
			Lead			11,340
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

				objects.push_back(engine::Entity{ static_cast<engine::Entity::value_type>((std::size_t)body->GetUserData()) });

				// Return true to continue the query.
				return true;
			}

		} query{ objects };

		b2AABB aabb{};

		aabb.lowerBound.Set(pos[0] - radius, pos[1] - radius);
		aabb.upperBound.Set(pos[0] + radius, pos[1] + radius);

		world.QueryAABB(&query, aabb);
	}

	void update()
	{
		const int32 velocityIterations = 6;
		const int32 positionIterations = 2;

		world.Step(timeStep, velocityIterations, positionIterations);

		// for (auto && movable : components.get<movables>()) // or something like that
		for (auto && actor : actors)
		{
			const auto & transform = actor.second.body->GetTransform();
			engine::graphics::data::ModelviewMatrix data = {
				core::maths::Matrix4x4f::translation(transform.p.x, transform.p.y, 0.f) *
				core::maths::Matrix4x4f::rotation(core::maths::radianf{transform.q.GetAngle()}, 0.f, 0.f, 1.f)
			};
			engine::graphics::renderer::update(actor.first, std::move(data));

			if (actor.second.debugRenderId!= ::engine::Entity::INVALID)
				engine::graphics::renderer::update(actor.second.debugRenderId, std::move(data));
		}
	}

	MoveResult update(const engine::Entity id, const MoveData & moveData)
	{
		b2Body *const body = actors.at(id).body;

	//	body->SetAngularVelocity(2);

		//const float ACC = -9.82f;
		//const float velY = moveData.velY + ACC*timeStep;
		b2Vec2 vel = body->GetLinearVelocity();

		b2Vec2 currentVel = convert(moveData.velXZ);
		vel.x += currentVel.x*1.f;

		body->SetLinearVelocity(vel);

		const b2Vec2 & pos = body->GetPosition();

		if (contactCounter.at(id).isGrounded())
		{
			// I check the velY < 0.f since otherwise jumps are interrupted. Contact updating is too slow.
			if (vel.y < 0.f)
			{
				return MoveResult(true, Point{{ (float)pos.x, (float)pos.y, (float)0.f }}, 0.f);
			}
		}

		// 
		return MoveResult(false, Point{{ (float)pos.x, (float)pos.y, (float)0.f}}, vel.y);
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

		/*b2Fixture *const fixture = */body->CreateFixture(&fixtureDef); // not used
		
		actors.emplace(id, PhysicsActor{ body, ::engine::Entity::INVALID });
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
			fixtureDef.friction = 10.f;//material.friction;
			fixtureDef.restitution = .1f;//material.restitution;
			//fixtureDef.isSensor = true;	// lower spape keeps track of contacts
			b2Fixture *const feets = body->CreateFixture(&fixtureDef);

			contactCounter.emplace(id, ShapeContactCounter{ feets });

			feets->SetUserData((void*)(std::size_t)static_cast<engine::Entity::value_type>(id));
			//feets->SetUserData(&contactCounter.at(id));
		}

		body->ResetMassData();
		body->SetFixedRotation(true);

		const ::engine::Entity debugId = ::engine::Entity::create();

		actors.emplace(id, PhysicsActor{body, debugId});

		// debug graphics
		{
			const std::size_t detail = 16;
			// this is awful...
			core::container::Buffer vertices_;
			vertices_.resize<float>(3 * (4 + (detail + 1) + (detail + 1)));
			std::size_t vertexi = 0;
			core::container::Buffer edges_;
			edges_.resize<uint16_t>(2 * (4 + detail + detail));
			std::size_t edgei = 0;
			// box
			const auto vertexboxi = vertexi;
			vertices_.data_as<float>()[vertexi * 3 + 0] = -halfRadius;
			vertices_.data_as<float>()[vertexi * 3 + 1] = -(halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
			edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexi + 1;
			vertexi++;
			edgei++;
			vertices_.data_as<float>()[vertexi * 3 + 0] = +halfRadius;
			vertices_.data_as<float>()[vertexi * 3 + 1] = -(halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
			edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexi + 1;
			vertexi++;
			edgei++;
			vertices_.data_as<float>()[vertexi * 3 + 0] = +halfRadius;
			vertices_.data_as<float>()[vertexi * 3 + 1] = +(halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
			edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexi + 1;
			vertexi++;
			edgei++;
			vertices_.data_as<float>()[vertexi * 3 + 0] = -halfRadius;
			vertices_.data_as<float>()[vertexi * 3 + 1] = +(halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
			edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexboxi;
			vertexi++;
			edgei++;
			// top hemisphere
			for (std::size_t i = 0; i < detail; i++)
			{
				const auto angle = float(i) / float(detail) * core::maths::constantf::pi;
				vertices_.data_as<float>()[vertexi * 3 + 0] = halfRadius * std::cos(angle);
				vertices_.data_as<float>()[vertexi * 3 + 1] = halfRadius * std::sin(angle) + (halfHeight - halfRadius);
				vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
				edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
				edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexi + 1;
				vertexi++;
				edgei++;
			}
			vertices_.data_as<float>()[vertexi * 3 + 0] = halfRadius * std::cos(core::maths::constantf::pi);
			vertices_.data_as<float>()[vertexi * 3 + 1] = halfRadius * std::sin(core::maths::constantf::pi) + (halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			vertexi++;
			// bottom hemisphere
			for (std::size_t i = 0; i < detail; i++)
			{
				const auto angle = float(i) / float(detail) * core::maths::constantf::pi + core::maths::constantf::pi;
				vertices_.data_as<float>()[vertexi * 3 + 0] = halfRadius * std::cos(angle);
				vertices_.data_as<float>()[vertexi * 3 + 1] = halfRadius * std::sin(angle) - (halfHeight - halfRadius);
				vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
				edges_.data_as<uint16_t>()[edgei * 2 + 0] = (uint16_t)vertexi;
				edges_.data_as<uint16_t>()[edgei * 2 + 1] = (uint16_t)vertexi + 1;
				vertexi++;
				edgei++;
			}
			vertices_.data_as<float>()[vertexi * 3 + 0] = halfRadius * std::cos(0.f);
			vertices_.data_as<float>()[vertexi * 3 + 1] = halfRadius * std::sin(0.f) + (halfHeight - halfRadius);
			vertices_.data_as<float>()[vertexi * 3 + 2] = 0.f;
			vertexi++;

			engine::graphics::data::LineC data = {
				core::maths::Matrix4x4f::identity(),
				std::move(vertices_),
				std::move(edges_),
				0xffff8844
			};
			engine::graphics::renderer::add(debugId, std::move(data)); // TODO: boundingbox_id
		}
	}

	void remove(const engine::Entity id)
	{
		const PhysicsActor & actor = actors.at(id);
		
		// remove debug object from Renderer
		if (actor.debugRenderId!= ::engine::Entity::INVALID)
			engine::graphics::renderer::remove(actor.debugRenderId);
		
		actors.erase(id);
		contactCounter.erase(id);
	}
}
}
