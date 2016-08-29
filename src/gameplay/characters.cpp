
#include "characters.hpp"

#include <gameplay/CharacterState.hpp>

#include <engine/physics/physics.hpp>

#include <core/container/CircleQueue.hpp>
#include <core/debug.hpp>

#include <unordered_map>

using core::maths::Vector2f;
using core::maths::Vector3f;

namespace
{
	using ::gameplay::characters::MovementState;
	using ::gameplay::characters::CharacterState;

	core::container::CircleQueueSRMW<engine::Entity, 100> queueCreate;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueRemove;
	core::container::CircleQueueSRMW<std::pair<engine::Entity, Vector3f>, 100> queueGrounded;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueFalling;
	core::container::CircleQueueSRMW<std::pair<engine::Entity, MovementState>, 100> queueMovement;

	std::unordered_map<engine::Entity, CharacterState> items;
}

namespace gameplay
{
namespace characters
{
	void update()
	{
		{
			engine::Entity id;

			// create
			while (queueCreate.try_pop2(id))
			{
				items.emplace(id, CharacterState());
			}

			// remove
			while (queueRemove.try_pop(id))
			{
				items.erase(id);
			}

			std::pair<engine::Entity, Vector3f> data;
		
			// update grounded state
			while (queueGrounded.try_pop2(data))
			{
				auto itr = items.find(data.first);

				if (itr != items.end())
				{
					itr->second.setGrounded(data.second);
				}
			}

			// update falling state
			while (queueFalling.try_pop(id))
			{
				auto itr = items.find(id);

				if (itr != items.end())
				{
					itr->second.clrGrounded();
				}
			}

			std::pair<engine::Entity, MovementState> state;

			// update movement state
			while (queueMovement.try_pop2(state))
			{
				auto itr = items.find(state.first);

				if (itr != items.end())
				{
					// update characters movement vector based on input
					itr->second.update(state.second);
				}
			}
		}

		// update the characters
		for (auto & item : items)
		{
			engine::physics::MoveResult res =
				engine::physics::update(
					item.first, 
					engine::physics::MoveData(
						item.second.movement(), 
						item.second.fallVel));

			item.second.fallVel = res.velY;
		}
	}

	void create(const engine::Entity id)
	{
		queueCreate.try_push(id);
	}

	void remove(const engine::Entity id)
	{
		queueRemove.try_push(id);
	}

	void postGrounded(const engine::Entity id, const core::maths::Vector3f normal)
	{
		queueGrounded.try_push(std::pair<engine::Entity, Vector3f>(id, normal));
	}

	void postFalling(const engine::Entity id)
	{
		queueFalling.try_push(id);
	}

	void postMovement(const engine::Entity id, const MovementState state)
	{
		queueMovement.try_push(std::pair<engine::Entity, MovementState>(id, state));
	}
}
}
