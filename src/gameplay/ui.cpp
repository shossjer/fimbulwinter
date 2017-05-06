
#include "ui.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <engine/hid/input.hpp>
#include <gameplay/gamestate.hpp>

#include <algorithm>

namespace
{
	struct GameMenu
	{
		engine::Entity entity;

		GameMenu(engine::Entity entity) : entity(entity) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ESCAPE:
					return true;
				default:
					return false;
				}
			default:
				return false;
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		5,
		std::array<GameMenu, 1>,
		std::array<GameMenu, 1>
	>
	components;

	struct translate_input
	{
		const engine::hid::Input & input;

		translate_input(const engine::hid::Input & input) : input(input) {}

		template <typename X>
		bool operator () (X & x)
		{
			return x.translate(input);
		}
	};

	constexpr std::size_t max_entities = 10;
	std::size_t n_entities = 0;
	engine::Entity entities[max_entities];

	void add(engine::Entity entity)
	{
		debug_assert(n_entities < max_entities);
		debug_assert(std::find(entities, entities + n_entities, entity) == entities + n_entities);

		entities[n_entities] = entity;
		n_entities++;
	}
	void remove(engine::Entity entity)
	{
		auto first = entities;
		auto last = entities + n_entities;

		auto it = std::find(first, last, entity);
		debug_assert(it != last);

		std::rotate(it, it + 1, last);
		n_entities--;
	}
	void set_focus(engine::Entity entity)
	{
		auto first = entities;
		auto last = entities + n_entities;

		auto it = std::find(first, last, entity);
		debug_assert(it != last);

		std::rotate(first, it, it + 1);
	}

	core::container::CircleQueueSRMW<engine::hid::Input, 100> queue_input;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_remove;
}

namespace gameplay
{
namespace ui
{
	void update()
	{
		// remove
		{
			engine::Entity entity;
			while (queue_remove.try_pop(entity))
			{
				components.remove(entity);
				remove(entity);
			}
		}

		// dispatch inputs
		{
			engine::hid::Input input;
			while (queue_input.try_pop(input))
			{
				for (std::size_t i = 0; i < n_entities; i++)
				{
					if (components.call(entities[i], translate_input{input}))
					// if (components.call(entities[i], [&input](auto & x){ return x.translate(input); }))
						break;
				}
			}
		}
	}

	void notify_input(const engine::hid::Input & input)
	{
		const auto res = queue_input.try_push(input);
		debug_assert(res);
	}

	void post_remove(engine::Entity entity)
	{
		const auto res = queue_remove.try_emplace(entity);
		debug_assert(res);
	}
}
}
