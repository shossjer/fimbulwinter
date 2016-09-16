
#include "characters.hpp"

#include <gameplay/CharacterState.hpp>

#include <engine/physics/physics.hpp>

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>

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

	core::container::Collection
	<
		engine::Entity,
		101,
		std::array<CharacterState, 50>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	components;

	struct clear_ground_state
	{
		void operator () (CharacterState & x)
		{
			x.clrGrounded();
		}
		template <typename X>
		void operator () (X & x) {}
	};
	struct update_ground_state
	{
		const Vector3f & normal;

		update_ground_state(const Vector3f & normal) : normal(normal) {}

		void operator () (CharacterState & x)
		{
			x.setGrounded(normal);
		}
		template <typename X>
		void operator () (X & x) {}
	};
	struct update_movement_state
	{
		const MovementState & state;

		update_movement_state(const MovementState & state) : state(state) {}

		void operator () (CharacterState & x)
		{
			x.update(state);
		}
		template <typename X>
		void operator () (X & x) {}
	};
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
			while (queueCreate.try_pop(id))
			{
				components.add(id, CharacterState());
			}

			// remove
			while (queueRemove.try_pop(id))
			{
				components.remove(id);
			}

			std::pair<engine::Entity, Vector3f> data;

			// update grounded state
			while (queueGrounded.try_pop(data))
			{
				components.call(data.first, update_ground_state{data.second});
			}

			// update falling state
			while (queueFalling.try_pop(id))
			{
				components.call(id, clear_ground_state{});
			}

			std::pair<engine::Entity, MovementState> state;

			// update movement state
			while (queueMovement.try_pop(state))
			{
				components.call(state.first, update_movement_state{state.second});
			}
		}

		// update the characters
		for (auto & component : components.get<CharacterState>())
		{
			auto res = engine::physics::update(components.get_key(component),
			                                   engine::physics::MoveData(component.movement(),
			                                                             component.fallVel));
			component.fallVel = res.velY;
		}
	}

	void create(const engine::Entity id)
	{
		const auto res = queueCreate.try_push(id);
		debug_assert(res);
	}

	void remove(const engine::Entity id)
	{
		const auto res = queueRemove.try_push(id);
		debug_assert(res);
	}

	void postGrounded(const engine::Entity id, const core::maths::Vector3f normal)
	{
		const auto res = queueGrounded.try_emplace(id, normal);
		debug_assert(res);
	}

	void postFalling(const engine::Entity id)
	{
		const auto res = queueFalling.try_push(id);
		debug_assert(res);
	}

	void postMovement(const engine::Entity id, const MovementState state)
	{
		const auto res = queueMovement.try_emplace(id, state);
		debug_assert(res);
	}
}
}
