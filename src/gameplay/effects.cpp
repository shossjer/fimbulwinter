
#include "effects.hpp"

#include "Effect.hpp"

#include <gameplay/ui.hpp>

#include <engine/graphics/viewer.hpp>
#include <engine/physics/queries.hpp>

#include <core/container/CircleQueue.hpp>

#include <atomic>
#include <memory>
#include <unordered_map>
#include <vector>

namespace gameplay
{
namespace effects
{
	using core::maths::Vector2f;
	using core::maths::Vector3f;

	class PlayerGravity : public Effect
	{
	private:

		const engine::Entity callerId;

	public:

		PlayerGravity(const engine::Entity id) : callerId(id) {}

	public:

		bool finished() override
		{
			return false;
		}

		void update() override
		{
			//// get centre position
			//const auto pp = engine::physics::query::positionOf(callerId);

			//// update physics
			//engine::physics::effect::acceleration(
			//	core::maths::Vector3f{ 0.f, 20.f, 0.f },
			//	pp,
			//	10.f);
		}
	};

	class MouseForce : public Effect
	{
	private:

		const engine::Entity callerId;

	public:

		MouseForce(const engine::Entity id)
			:
			callerId(id)
		{
		}

	public:

		bool finished() override
		{
			return false;
		}

		void update() override
		{
			//// get centre position
			//const auto pp = engine::physics::query::positionOf(callerId);

			//// get mouse coordinates
			//const auto mc = gameplay::ui::mouseCoords(); // input::mouseCoords();

			//Vector3f mp;
			//engine::graphics::viewer::from_screen_to_world(Vector2f{ (float)mc.x, (float)mc.y }, mp);

			//// delta from player to cursor
			//const core::maths::Vector3f d = mp - pp;

			//// multiplier to get value in force range (~10kN)
			//const float S = 1000.f;

			//// using the cursor distance as radius for AoE
			//const auto radius = length(d);

			//// update physics
			//engine::physics::effect::force(d*S, pp, radius);
		}
	};

	namespace
	{
		core::container::CircleQueueSRMW<std::pair<engine::Entity, std::unique_ptr<Effect> >, 20> queueAdd;

		core::container::CircleQueueSRMW<engine::Entity, 20> queueRemove;

		std::unordered_map<Id, std::unique_ptr<Effect> > items;
	}

	void update()
	{
		{
			// create
			std::pair<engine::Entity, std::unique_ptr<Effect>> addEffect;
			while (queueAdd.try_pop(addEffect))
			{
				items.emplace(addEffect.first, std::move(addEffect.second));
			}
		}
		{
			// remove
			engine::Entity remId;
			while (queueRemove.try_pop(remId))
			{
				items.erase(remId);
			}
		}

		// update
		for (auto i = items.begin(); i!= items.end();)
		{
			(*i).second->update();

			if ((*i).second->finished())
			{
				i = items.erase(i);
			}
			else
			{
				++i;
			}
		}
	}

	engine::Entity create(const Type type, const engine::Entity callerId)
	{
		printf("Player ability CREATING!\n");

		const engine::Entity id = engine::Entity::create();

		switch (type)
		{
		case Type::PLAYER_GRAVITY:
			
			queueAdd.try_push(std::make_pair(id, std::unique_ptr<Effect>(new PlayerGravity(callerId))));
			break;

		case Type::PLAYER_MOUSE_FORCE:

			queueAdd.try_push(std::make_pair(id, std::unique_ptr<Effect>(new MouseForce(callerId))));
			break;
		}

		return id;
	}

	void remove(const engine::Entity id)
	{
		printf("Player ability REMOVING!\n");

		queueRemove.try_push(id);
	}
}
}
