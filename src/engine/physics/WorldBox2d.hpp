
#ifndef ENGINE_PHYSICS_BOX2D_WORLD_HPP
#define ENGINE_PHYSICS_BOX2D_WORLD_HPP

#include <config.h>

#if PHYSICS_USE_BOX2D

#include <engine/physics/WorldBase.hpp>

#include <Box2D/Box2D.h>

#include <core/maths/Vector.hpp>

namespace engine
{
namespace physics
{
	class WorldBox2d : public WorldBase<b2Body>, public b2ContactListener
	{
	protected:

		b2Vec2 convert(const core::maths::Vector3f val)
		{
			core::maths::Vector3f::array_type buffer;
			val.get_aligned(buffer);
			return b2Vec2 {buffer[0], buffer[1]};
		}
		core::maths::Vector3f convert(const b2Vec2 val)
		{
			return core::maths::Vector3f {val.x, val.y, 0.f};
		}

		// Contact counter for characters to determine falling / grounded
		struct ContactCounter
		{
		private:
			unsigned int contacts;

		public:
			b2Fixture *const fixture;

		public:
			ContactCounter(b2Fixture *const fixture)
				:
				contacts {0},
				fixture {fixture}
			{
			}

			bool isGrounded() const { return this->contacts!=0; }

			void increment() { this->contacts++; }
			void decrement() { this->contacts--; }
		};

	private:

		std::unordered_map<engine::Entity, ContactCounter> contactCounter;

		b2World world;

	public:

		WorldBox2d(const float timeStep) : WorldBase(timeStep), world(b2Vec2 {0.f, -9.82f}) {}

		void initialize(const Callbacks & callbacks)
		{
			// Used during contact callbacks
			this->callbacks = &callbacks;

			// Register to Contact callbacks from box2d
			this->world.SetContactListener(this);
		}

	public:

		// creates body based on definition and sets Entity id
		b2Body * createBody(const engine::Entity id, b2BodyDef & def)
		{
			b2Body * body = this->world.CreateBody(&def);

			body->SetUserData((void*) (std::size_t)static_cast<engine::Entity::value_type>(id));

			return body;
		}

	public:

		// Update the physics world.
		// Should be called after any additional input to the world has been made.
		// Like:
		//		applying forces and impulses
		//		creating and removing Objects
		void update(const float timeStep);

		// Update movement of Character.
		// Call this method before calling world update.
		void update(const engine::Entity id, core::maths::Vector3f data);

	public:

		WorldBase::Actor & create(const engine::Entity id, const MaterialData & material, const BoxData & data);

		WorldBase::Actor & create(const engine::Entity id, const MaterialData & material, const CharacterData & data);

		void remove(const engine::Entity id);

		::core::maths::Vector3f positionOf(const engine::Entity id)
		{
			const auto pos = getBody(id)->GetPosition();

			return ::core::maths::Vector3f(pos.x, pos.y, 0.f);
		}

		void positionOf(const engine::Entity id, Vector3f & pos, Vector3f & velocity, float & angle)
		{
			const auto * const body = getBody(id);

			const auto point = body->GetPosition();
			const auto vel = body->GetLinearVelocity();
			angle = body->GetAngle();

			pos = Vector3f {point.x, point.y, 0.f};

			velocity = Vector3f {vel.x, vel.y, 0.f};
		}

	private:

		void BeginContact(const b2Fixture *const fixA, const b2Fixture *const fixB, b2Contact *const contact);

		void EndContact(const b2Fixture *const fixA, const b2Fixture *const fixB, const b2Contact *const contact);

	public:
		// overrides callback function BeginContact from b2ContactListener
		void BeginContact(b2Contact* contact) override;

		// overrides callback function EndContact from b2ContactListener
		void EndContact(b2Contact* contact) override;
	};
}
}

#endif // ENGINE_PHYSICS_BOX2D_WORLD_HPP

#endif // PHYSICS_USE_BOX2D