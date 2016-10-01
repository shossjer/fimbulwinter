
#include "ui.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <engine/hid/input.hpp>
#include <gameplay/characters.hpp>

#include <algorithm>

namespace
{
	namespace mouse
	{
		gameplay::ui::coords_t frameCoords{ 0, 0 };
		gameplay::ui::coords_t frameDelta{ 0, 0 };
		gameplay::ui::coords_t updatedCoords{ 0, 0 };
	}

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

	struct Player
	{
		engine::Entity entity;

		Player(engine::Entity entity) : entity(entity) {}

		bool translate(const engine::hid::Input & input)
		{
			static bool is_down_left = false;
			static bool is_down_right = false;

			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					if (!is_down_left)
					{
						is_down_left = true;
						gameplay::characters::post_command(entity, gameplay::characters::Command::GO_LEFT);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					if (!is_down_right)
					{
						is_down_right = true;
						gameplay::characters::post_command(entity, gameplay::characters::Command::GO_RIGHT);
					}
					return true;
				default:
					return false;
				}
			case engine::hid::Input::State::MOVE:
				mouse::updatedCoords = gameplay::ui::coords_t{ input.getCursor().x, input.getCursor().y };
				return true;
			case engine::hid::Input::State::UP:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					is_down_left = false;
					if (!is_down_right)
					{
						gameplay::characters::post_command(entity, gameplay::characters::Command::STOP_ITS_HAMMER_TIME);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					is_down_right = false;
					if (!is_down_left)
					{
						gameplay::characters::post_command(entity, gameplay::characters::Command::STOP_ITS_HAMMER_TIME);
					}
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
		101,
		std::array<GameMenu, 1>,
		std::array<Player, 1>
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

	core::container::CircleQueueSRMW<engine::Entity, 10> queue_add_player;
	core::container::CircleQueueSRMW<engine::hid::Input, 100> queue_input;
	core::container::CircleQueueSRMW<engine::Entity, 100> queue_remove;
}

namespace gameplay
{
	namespace ui
	{
		void update()
		{
			// add player
			{
				engine::Entity entity;
				while (queue_add_player.try_pop(entity))
				{
					components.emplace<Player>(entity, entity);
					add(entity);
				}
			}
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

			// update input state data
			const auto uc = mouse::updatedCoords;

			//
			mouse::frameDelta = coords_t{ static_cast<short>(uc.x - mouse::frameCoords.x),
			                              static_cast<short>(uc.y - mouse::frameCoords.y) };
			mouse::frameCoords = uc;
		}

		void notify_input(const engine::hid::Input & input)
		{
			const auto res = queue_input.try_push(input);
			debug_assert(res);
		}

		void post_add_player(engine::Entity entity)
		{
			const auto res = queue_add_player.try_emplace(entity);
			debug_assert(res);
		}
		void post_remove(engine::Entity entity)
		{
			const auto res = queue_remove.try_emplace(entity);
			debug_assert(res);
		}

		coords_t mouseCoords()
		{
			return mouse::frameCoords;
		}
		coords_t mouseDelta()
		{
			return mouse::frameDelta;
		}
	}
}
