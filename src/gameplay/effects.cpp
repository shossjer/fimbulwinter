
#include "effects.hpp"

#include "Effect.hpp"

#include <engine/physics/effects.hpp>
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
			// get centre position
			const Point point = engine::physics::query::positionOf(callerId);

			// get mouse coordinates in the world

			// update physics

			engine::physics::effect::acceleration(
				Vector{{ 0.f, 20.f, 0.f }},
				point,
				10.f);
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
			while (queueAdd.try_pop2(addEffect))
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
