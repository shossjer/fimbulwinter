
#include <config.h>

#include "Callbacks.hpp"
#include "helper.hpp"
#include "physics.hpp"
#include "queries.hpp"
#include "MaterialData.hpp"

#if PHYSICS_USE_PHYSX

#else
#include "WorldBox2d.hpp"
#endif

#include <core/container/CircleQueue.hpp>
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
	constexpr float timeStep = 1.f / 60.f;

#if PHYSICS_USE_PHYSX
	using Body = px::Body;
#else
	using Body = b2Body;
#endif
	using Actor = WorldBase<Body>::Actor;

	namespace
	{
		// The Physics World - home of bouncing Boxes
#if PHYSICS_USE_PHYSX

#else
		WorldBox2d world {timeStep};
#endif
		// All defined physics materials. Contains density, friction and restitution
		std::unordered_map<Material, MaterialData> materials;

		core::container::CircleQueueSRMW<std::pair<engine::Entity, core::maths::radianf>, 100> queue_set_headings;
		core::container::CircleQueueSRMW<std::pair<engine::Entity, core::maths::Vector3f>, 100> queue_movements;
	}

	namespace query
	{
		::core::maths::Vector3f positionOf(const engine::Entity id)
		{
			return world.positionOf(id);
		}

		void positionOf(const engine::Entity id, Point & pos, Vector & velocity, float & angle)
		{
			world.positionOf(id, pos, velocity, angle);
		}
	}

	void initialize(const Callbacks & callbacks)
	{
		// Register to callback events from physics
		// * Contact - called when falling / grounded state changes.
		// * Collision - ...
		world.initialize(callbacks);

#ifdef PHYSICS_USE_PHYSX

#else
		const engine::Entity id = engine::Entity::create();

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
		b2Body* body = world.createBody(id, bodyDef);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &chain;
		fixtureDef.density = 1.f;
		fixtureDef.friction = .3f;

		body->CreateFixture(&fixtureDef);

		// debug graphics
		{
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
#endif // PHYSICS_USE_PHYSX

		// Water (salt)	1,030
		// Plastics		1,175
		// Concrete		2,000
		// Iron			7, 870
		// Lead			11,340
		// setup materials
		materials.emplace(Material::MEETBAG, MaterialData(1000.f, .5f, .4f));
		materials.emplace(Material::STONE, MaterialData(2000.f, .4f, .05f));
		materials.emplace(Material::SUPER_RUBBER, MaterialData(1200.f, 1.0f, 1.0f));
		materials.emplace(Material::WOOD, MaterialData(700.f, .6f, .2f));
	}

	void teardown()
	{

	}

	void update()
	{
		// poll heading queue
		{
			std::pair<engine::Entity, core::maths::radianf> data;
			while (queue_set_headings.try_pop(data))
			{
				auto & actor = world.getActor(data.first);
				actor.heading = data.second;
			}
		}
		// poll movement queue
		{
			std::pair<engine::Entity, core::maths::Vector3f> data;
			while (queue_movements.try_pop(data))
			{
				world.update(data.first, data.second);
			}
		}
		
		// Update the physics world
		world.update(timeStep);
	}

	void post_movement(engine::Entity id, core::maths::Vector3f movement)
	{
		const auto res = queue_movements.try_emplace(id, movement);
		debug_assert(res);
	}
	void post_set_heading(engine::Entity id, core::maths::radianf rotation)
	{
		const auto res = queue_set_headings.try_emplace(id, rotation);
		debug_assert(res);
	}

	void create(const engine::Entity id, const BoxData & data)
	{
		const MaterialData & material = materials.at(data.material);

		Actor & actor = world.create(id, material, data);
		
		//actor.debugRenderId
	}

	void create(const engine::Entity id, const CharacterData & data)
	{
		const MaterialData & material = materials.at(data.material);
		
		Actor & actor = world.create(id, material, data);

		const ::engine::Entity debugId = ::engine::Entity::create();

		actor.debugRenderId = debugId;

		// debug graphics
		{
			const float halfRadius = data.radius*.5f;
			const float halfHeight = data.height*.5f;

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
		const Actor & actor = world.getActor(id);
		
		// remove debug object from Renderer
		if (actor.debugRenderId!= ::engine::Entity::INVALID)
			engine::graphics::renderer::remove(actor.debugRenderId);
		
		world.remove(id);
	}
}
}
