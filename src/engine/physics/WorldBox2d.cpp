
#include "WorldBox2d.hpp"

#if PHYSICS_USE_BOX2D

#include <engine/graphics/renderer.hpp>

namespace engine
{
namespace physics
{
	static b2Vec2 convert(const core::maths::Vector3f val)
	{
		core::maths::Vector3f::array_type buffer;
		val.get_aligned(buffer);
		return b2Vec2 {buffer[0], buffer[1]};
	}

	void WorldBox2d::update(const float timeStep)
	{
		constexpr int32 velocityIterations = 6;
		constexpr int32 positionIterations = 2;

		this->world.Step(timeStep, velocityIterations, positionIterations);

		// for (auto && movable : components.get<movables>()) // or something like that
		for (auto && actor:actors)
		{
			const auto & transform = actor.second.body->GetTransform();
			engine::graphics::data::ModelviewMatrix data = {
				core::maths::Matrix4x4f::translation(transform.p.x, transform.p.y, 0.f) *
				core::maths::Matrix4x4f::rotation(core::maths::radianf {transform.q.GetAngle()}, 0.f, 0.f, 1.f) *
				core::maths::Matrix4x4f::rotation(actor.second.heading, 0.f, 1.f, 0.f)
			};
			engine::graphics::renderer::update(actor.first, std::move(data));

			if (actor.second.debugRenderId!=::engine::Entity::INVALID)
				engine::graphics::renderer::update(actor.second.debugRenderId, std::move(data));
		}
	}

	void WorldBox2d::update(const engine::Entity id, core::maths::Vector3f movement)
	{
		auto & actor = getActor(id);
		// actor.movement = data.second;
		const auto vel = actor.body->GetLinearVelocity();
		// vel.x = convert(data.second).x;
		core::maths::Vector3f::array_type buffer;
		movement.get_aligned(buffer);

		const float mx = -buffer[1]*std::sin(actor.heading.get())/8;
		const float my = buffer[2]/8;

		// temp
		if (mx==0.f && my==0.f) return;

		const auto mass = actor.body->GetMass();

		const bool stabile = true;

		if (stabile)
		{
			// Make sure object has correct velocity at end of frame
			// the object will Only move half the delta distance from
			// previous frame; this frame.
			// If the object was still last frame and is told to mode
			// 10 mm this frame; it will only move 5 mm. If it is told
			// to move 10 mm the next frame, then it will move 10 mm.
			// If the third frame it is told to move 15 mm it will
			// then move 12.5 mm... so half the delta distance.
			const b2Vec2 goalVel(mx/timeStep, my/timeStep);
			const b2Vec2 deltaVel = goalVel-vel;

			const b2Vec2 acc(deltaVel.x/timeStep, deltaVel.y/timeStep);

			debug_printline(0xffffffff , "acc: ", acc.x, " ", acc.y);

			actor.body->ApplyForceToCenter(b2Vec2(acc.x*mass, acc.y*mass), true);
		}
		else
		{
			// calculate needed accelleration to achieve movement
			// s = v0 * t + a * t * t / 2
			// a = 1/t*t (2*s - 2*v0*t)
			// a = 2*s/(t*t) - 2*v0/t
			// use the Force
			b2Vec2 acc;
			acc.x = 2.f*mx/(timeStep*timeStep)-2.f*vel.x/timeStep;
			acc.y = 2.f*my/(timeStep*timeStep)-2.f*vel.y/timeStep;

			debug_printline(0xffffffff, "acc: ", acc.x, " ", acc.y);

			actor.body->ApplyForceToCenter(b2Vec2(acc.x*mass, acc.y*mass), true);
		}
	}

	template<class T>
	auto myfunc(T && t1) -> decltype(auto)
	{
		return t1*t1;
	}

	auto WorldBox2d::create(const engine::Entity id, const MaterialData & material, const BoxData & data) -> WorldBase::Actor &
	{
		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		const auto pos = convert(data.pos);
		bodyDef.position.Set(pos.x, pos.y);

		b2Body*const body = createBody(id, bodyDef);

		b2PolygonShape dynamicBox;
		const auto size = convert(data.size);
		dynamicBox.SetAsBox(size.x, size.y);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &dynamicBox;
		fixtureDef.density = material.density;
		fixtureDef.friction = material.friction;
		fixtureDef.restitution = material.restitution;

		/*b2Fixture *const fixture = */body->CreateFixture(&fixtureDef); // not used

		return WorldBase::create(id, body);
	}

	auto WorldBox2d::create(const engine::Entity id, const MaterialData & material, const CharacterData & data) -> WorldBase::Actor &
	{
		const float halfRadius = data.radius*.5f;
		const float halfHeight = data.height*.5f;

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		const auto pos = convert(data.pos);
		bodyDef.position.Set(pos.x, pos.y);

		b2Body*const body = createBody(id, bodyDef);
		{
			b2PolygonShape shape;
			shape.SetAsBox(halfRadius, halfHeight-halfRadius);

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
			shape.m_p = b2Vec2(0.f, (halfHeight-halfRadius));	// adjust position

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
			shape.m_p = b2Vec2(0.f, (-halfHeight+halfRadius));	// adjust position

			b2FixtureDef fixtureDef;
			fixtureDef.shape = &shape;
			fixtureDef.density = material.density;
			fixtureDef.friction = material.friction;
			fixtureDef.restitution = material.restitution;
			//fixtureDef.isSensor = true;

			// lower sphere keeps track of contacts
			b2Fixture *const feets = body->CreateFixture(&fixtureDef);

			this->contactCounter.emplace(id, ContactCounter {feets});

			feets->SetUserData((void*) (std::size_t)static_cast<engine::Entity::value_type>(id));
		}

		body->ResetMassData();
		body->SetFixedRotation(true);

		return WorldBase::create(id, body);
	}

	void WorldBox2d::remove(const engine::Entity id)
	{
		this->actors.erase(id);
		this->contactCounter.erase(id);
	}

	void WorldBox2d::BeginContact(const b2Fixture *const fixA, const b2Fixture *const fixB, b2Contact *const contact)
	{
		auto id = engine::Entity {static_cast<engine::Entity::value_type>((std::size_t)fixA->GetUserData())};

		auto itr = this->contactCounter.find(id);

		if (itr!=this->contactCounter.end())
		{
			// change the restitution to prevent bounce
			contact->SetRestitution(0.f);

			itr->second.increment();

			// TODO: calculate ground normal if more then one contact
			this->callbacks->onGrounded(id, convert(contact->GetManifold()->localNormal));
		}
	}

	void WorldBox2d::EndContact(const b2Fixture *const fixA, const b2Fixture *const fixB, const b2Contact *const contact)
	{
		auto id = engine::Entity {static_cast<engine::Entity::value_type>((std::size_t)fixA->GetUserData())};

		auto itr = this->contactCounter.find(id);

		if (itr!=this->contactCounter.end())
		{
			itr->second.decrement();

			if (itr->second.isGrounded())
			{
				// TODO: calculate ground normal if more then one contact
				this->callbacks->onGrounded(id, convert(contact->GetManifold()->localNormal));
			}
			else
			{
				this->callbacks->onFalling(id);
			}
		}
	}

	void WorldBox2d::BeginContact(b2Contact* contact)
	{
		if (contact->GetFixtureA()->GetUserData())
		{
			BeginContact(contact->GetFixtureA(), contact->GetFixtureB(), contact);
		}

		if (contact->GetFixtureB()->GetUserData())
		{
			BeginContact(contact->GetFixtureB(), contact->GetFixtureA(), contact);
		}

		debug_printline(0xffffffff, "contact type: ", contact->GetManifold()->type, ", normal: ", contact->GetManifold()->localNormal.x, ", points:", contact->GetManifold()->localNormal.y, " (grounded)", contact->GetManifold()->pointCount);
	}

	void WorldBox2d::EndContact(b2Contact* contact)
	{
		if (contact->GetFixtureA()->GetUserData())
		{
			EndContact(contact->GetFixtureA(), contact->GetFixtureB(), contact);
		}

		if (contact->GetFixtureB()->GetUserData())
		{
			EndContact(contact->GetFixtureB(), contact->GetFixtureA(), contact);
		}

		debug_printline(0xffffffff, "contact type: ", contact->GetManifold()->type, ", normal: ", contact->GetManifold()->localNormal.x, ", points:", contact->GetManifold()->localNormal.y, " (falling)", contact->GetManifold()->pointCount);
	}
}
}

#endif // PHYSICS_USE_BOX2D