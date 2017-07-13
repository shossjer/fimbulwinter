
#include "ui.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/debug.hpp>

#include <engine/Command.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/hid/input.hpp>

#include <algorithm>
#include <tuple>
#include <vector>

namespace engine
{
namespace graphics
{
namespace renderer
{
	extern void notify(Cursor && data);
}
}
}

namespace gameplay
{
namespace gamestate
{
	extern void post_command(engine::Entity entity, engine::Command command);
}
}

namespace
{
	struct dimension_t
	{
		int32_t width, height;
	};

	dimension_t dimension = { 0, 0 };

	////////////////////////////////////////////////////////////
	//
	//  Context
	//
	////////////////////////////
	struct ContextEntities
	{
		engine::Asset context;
		std::vector<engine::Entity> entities;
		std::vector<int> priorities;

		ContextEntities(engine::Asset context) : context(context) {}

		void add(engine::Entity entity, int priority)
		{
			const auto priorities_it = std::upper_bound(std::begin(priorities), std::end(priorities), priority);
			const auto entities_it = std::begin(entities) + std::distance(std::begin(priorities), priorities_it);

			entities.insert(entities_it, entity);
			priorities.insert(priorities_it, priority);
		}
		void remove(engine::Entity entity)
		{
			const auto entities_it = std::lower_bound(std::begin(entities), std::end(entities), entity);
			const auto priorities_it = std::begin(priorities) + std::distance(std::begin(entities), entities_it);

			debug_assert(entities_it != std::end(entities));
			std::rotate(entities_it, entities_it + 1, std::end(entities));
			std::rotate(priorities_it, priorities_it + 1, std::end(priorities));

			entities.pop_back();
			priorities.pop_back();
		}
	};

	std::vector<ContextEntities> contexts;
	engine::Asset active_context;

	int find_context(engine::Asset context)
	{
		for (int i = 0; i < contexts.size(); i++)
			if (contexts[i].context == context)
				return i;
		return -1;
	}

	void add_context(engine::Asset context)
	{
		debug_assert(find_context(context) < 0);
		contexts.emplace_back(context);
	}
	void remove_context(engine::Asset context)
	{
		debug_assert(context != active_context);
		const int i = find_context(context);
		debug_assert(i >= 0);
		const int last = contexts.size() - 1;
		if (i < last)
			contexts[i] = std::move(contexts[last]);
		contexts.pop_back();
	}

	void set_active_context(engine::Asset context)
	{
		debug_printline(0xffffffff, "Switching context: ", static_cast<int>(active_context), " -> ", static_cast<int>(context));
		active_context = context;
	}
	ContextEntities & get_active_context()
	{
		return contexts[find_context(active_context)];
	}
	////////////////////////////////////////////////////////////
	//
	//  Component
	//
	////////////////////////////
	struct ContextSwitch
	{
		engine::Entity entity;
		engine::hid::Input::Button button;
		engine::Asset context;

		ContextSwitch(engine::Entity entity, engine::hid::Input::Button button, engine::Asset context) : entity(entity), button(button), context(context) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				if (input.getButton() == button)
				{
					set_active_context(context);
					gameplay::gamestate::post_command(entity, engine::Command::CONTEXT_CHANGED);
					return true;
				}
				return false;
			default:
				return false;
			}
		}
	};

	struct Flycontrol
	{
		engine::Entity entity;

		bool is_down_left_turn = false;
		bool is_down_right_turn = false;
		bool is_down_up_turn = false;
		bool is_down_down_turn = false;
		bool is_down_left_move = false;
		bool is_down_right_move = false;
		bool is_down_up_move = false;
		bool is_down_down_move = false;
		bool is_down_left_roll = false;
		bool is_down_right_roll = false;

		Flycontrol(engine::Entity entity) : entity(entity) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					if (!is_down_left_turn)
					{
						is_down_left_turn = true;
						gameplay::gamestate::post_command(entity, engine::Command::TURN_LEFT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					if (!is_down_right_turn)
					{
						is_down_right_turn = true;
						gameplay::gamestate::post_command(entity, engine::Command::TURN_RIGHT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWUP:
					if (!is_down_up_turn)
					{
						is_down_up_turn = true;
						gameplay::gamestate::post_command(entity, engine::Command::TURN_UP_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWDOWN:
					if (!is_down_down_turn)
					{
						is_down_down_turn = true;
						gameplay::gamestate::post_command(entity, engine::Command::TURN_DOWN_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_A:
					if (!is_down_left_move)
					{
						is_down_left_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_LEFT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_D:
					if (!is_down_right_move)
					{
						is_down_right_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_RIGHT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_W:
					if (!is_down_up_move)
					{
						is_down_up_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_UP_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_S:
					if (!is_down_down_move)
					{
						is_down_down_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_DOWN_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_Q:
					if (!is_down_left_roll)
					{
						is_down_left_roll = true;
						gameplay::gamestate::post_command(entity, engine::Command::ROLL_LEFT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_E:
					if (!is_down_right_roll)
					{
						is_down_right_roll = true;
						gameplay::gamestate::post_command(entity, engine::Command::ROLL_RIGHT_DOWN);
					}
					return true;
				default:
					return false;
				}
			case engine::hid::Input::State::UP:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					is_down_left_turn = false;
					gameplay::gamestate::post_command(entity, engine::Command::TURN_LEFT_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					is_down_right_turn = false;
					gameplay::gamestate::post_command(entity, engine::Command::TURN_RIGHT_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWUP:
					is_down_up_turn = false;
					gameplay::gamestate::post_command(entity, engine::Command::TURN_UP_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWDOWN:
					is_down_down_turn = false;
					gameplay::gamestate::post_command(entity, engine::Command::TURN_DOWN_UP);
					return true;
				case engine::hid::Input::Button::KEY_A:
					is_down_left_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_LEFT_UP);
					return true;
				case engine::hid::Input::Button::KEY_D:
					is_down_right_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_RIGHT_UP);
					return true;
				case engine::hid::Input::Button::KEY_W:
					is_down_up_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_UP_UP);
					return true;
				case engine::hid::Input::Button::KEY_S:
					is_down_down_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_DOWN_UP);
					return true;
				case engine::hid::Input::Button::KEY_Q:
					is_down_left_roll = false;
					gameplay::gamestate::post_command(entity, engine::Command::ROLL_LEFT_UP);
					return true;
				case engine::hid::Input::Button::KEY_E:
					is_down_right_roll = false;
					gameplay::gamestate::post_command(entity, engine::Command::ROLL_RIGHT_UP);
					return true;
				default:
					return false;
				}
				return false;
			default:
				return false;
			}
		}
	};

	struct Pancontrol
	{
		engine::Entity entity;

		bool is_down_left_move = false;
		bool is_down_right_move = false;
		bool is_down_up_move = false;
		bool is_down_down_move = false;

		Pancontrol(engine::Entity entity) : entity(entity) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					if (!is_down_left_move)
					{
						is_down_left_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_LEFT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					if (!is_down_right_move)
					{
						is_down_right_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_RIGHT_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWUP:
					if (!is_down_up_move)
					{
						is_down_up_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_UP_DOWN);
					}
					return true;
				case engine::hid::Input::Button::KEY_ARROWDOWN:
					if (!is_down_down_move)
					{
						is_down_down_move = true;
						gameplay::gamestate::post_command(entity, engine::Command::MOVE_DOWN_DOWN);
					}
					return true;
				default:
					return false;
				}
			case engine::hid::Input::State::UP:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::KEY_ARROWLEFT:
					is_down_left_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_LEFT_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWRIGHT:
					is_down_right_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_RIGHT_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWUP:
					is_down_up_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_UP_UP);
					return true;
				case engine::hid::Input::Button::KEY_ARROWDOWN:
					is_down_down_move = false;
					gameplay::gamestate::post_command(entity, engine::Command::MOVE_DOWN_UP);
					return true;
				default:
					return false;
				}
				return false;
			default:
				return false;
			}
		}
	};

	struct RenderHover
	{
		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::MOVE:
				notify(engine::graphics::renderer::Cursor{input.getCursor().x, input.getCursor().y});
				return true;
			default:
				return false;
			}
		}
	};

	struct RenderSelect
	{
		engine::Entity entity;

		RenderSelect(engine::Entity entity) : entity(entity) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				switch (input.getButton())
				{
				case engine::hid::Input::Button::MOUSE_LEFT:
					engine::graphics::renderer::post_select(input.getCursor().x, input.getCursor().y, entity);
					return true;
				default:
					return false;
				}
			default:
				return false;
			}
		}
	};

	struct RenderSwitch
	{
		engine::hid::Input::Button button;

		bool is_down = false;

		RenderSwitch(engine::hid::Input::Button button) : button(button) {}

		bool translate(const engine::hid::Input & input)
		{
			switch (input.getState())
			{
			case engine::hid::Input::State::DOWN:
				if (input.getButton() == button)
				{
					if (!is_down)
					{
						is_down = true;
						engine::graphics::renderer::toggle_down();
					}
					return true;
				}
				return false;
			case engine::hid::Input::State::UP:
				if (input.getButton() == button)
				{
					is_down = false;
					engine::graphics::renderer::toggle_up();
					return true;
				}
				return false;
			default:
				return false;
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		81,
		std::array<ContextSwitch, 10>,
		std::array<Flycontrol, 10>,
		std::array<Pancontrol, 10>,
		std::array<RenderHover, 10>,
		std::array<RenderSelect, 10>,
		std::array<RenderSwitch, 10>
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

	core::container::ExchangeQueueSRSW<dimension_t> queue_dimension;

	core::container::CircleQueueSRMW<engine::hid::Input, 100> queue_input;

	core::container::CircleQueueSRMW<engine::Asset, 10> queue_add_context;
	core::container::CircleQueueSRMW<engine::Asset, 10> queue_remove_context;
	core::container::CircleQueueSRMW<engine::Asset, 10> queue_activate_context;

	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::hid::Input::Button, engine::Asset>, 10> queue_add_contextswitch;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Entity>, 10> queue_add_flycontrol;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::Entity>, 10> queue_add_pancontrol;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, int>, 10> queue_add_renderhover;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, int>, 10> queue_add_renderselect;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, engine::hid::Input::Button>, 10> queue_add_renderswitch;
	core::container::CircleQueueSRMW<std::tuple<engine::Entity, int>, 10> queue_remove;

	core::container::CircleQueueSRMW<std::tuple<engine::Asset, engine::Entity, int>, 50> queue_bind;
	core::container::CircleQueueSRMW<std::tuple<engine::Asset, engine::Entity>, 50> queue_unbind;
}

namespace gameplay
{
namespace ui
{
	void create()
	{
		add_context("null");
		set_active_context("null");
	}

	void destroy()
	{
	}

	void update()
	{
		//
		// read notifications
		//
		dimension_t notification_dimension;
		if (queue_dimension.try_pop(notification_dimension))
		{
			dimension = notification_dimension;
		}

		// context
		{
			engine::Asset context;
			while (queue_add_context.try_pop(context))
			{
				add_context(context);
			}
			while (queue_remove_context.try_pop(context))
			{
				remove_context(context);
			}
			while (queue_activate_context.try_pop(context))
			{
				set_active_context(context);
			}
		}

		// add/remove
		{
			std::tuple<engine::Entity, engine::hid::Input::Button, engine::Asset> contextswitch_args;
			while (queue_add_contextswitch.try_pop(contextswitch_args))
			{
				components.emplace<ContextSwitch>(std::get<0>(contextswitch_args), std::get<0>(contextswitch_args), std::get<1>(contextswitch_args), std::get<2>(contextswitch_args));
			}
			std::tuple<engine::Entity, engine::Entity> flycontrol_args;
			while (queue_add_flycontrol.try_pop(flycontrol_args))
			{
				components.emplace<Flycontrol>(std::get<0>(flycontrol_args), std::get<1>(flycontrol_args));
			}
			std::tuple<engine::Entity, engine::Entity> pancontrol_args;
			while (queue_add_pancontrol.try_pop(pancontrol_args))
			{
				components.emplace<Pancontrol>(std::get<0>(pancontrol_args), std::get<1>(pancontrol_args));
			}
			std::tuple<engine::Entity, int> renderhover_args;
			while (queue_add_renderhover.try_pop(renderhover_args))
			{
				components.emplace<RenderHover>(std::get<0>(renderhover_args));
			}
			std::tuple<engine::Entity, int> renderselect_args;
			while (queue_add_renderselect.try_pop(renderselect_args))
			{
				components.emplace<RenderSelect>(std::get<0>(renderselect_args), std::get<0>(renderselect_args));
			}
			std::tuple<engine::Entity, engine::hid::Input::Button> renderswitch_args;
			while (queue_add_renderswitch.try_pop(renderswitch_args))
			{
				components.emplace<RenderSwitch>(std::get<0>(renderswitch_args), std::get<1>(renderswitch_args));
			}
			std::tuple<engine::Entity, int> remove_args;
			while (queue_remove.try_pop(remove_args))
			{
				components.remove(std::get<0>(remove_args));
			}
		}

		// bind/unbind
		{
			std::tuple<engine::Asset, engine::Entity, int> bind_args;
			while (queue_bind.try_pop(bind_args))
			{
				contexts[find_context(std::get<0>(bind_args))].add(std::get<1>(bind_args), std::get<2>(bind_args));
			}
			std::tuple<engine::Asset, engine::Entity> unbind_args;
			while (queue_unbind.try_pop(unbind_args))
			{
				contexts[find_context(std::get<0>(unbind_args))].remove(std::get<1>(bind_args));
			}
		}

		// dispatch inputs
		{
			engine::hid::Input input;
			while (queue_input.try_pop(input))
			{
				for (const auto entity : contexts[find_context(active_context)].entities)
				{
					if (components.call(entity, translate_input{input}))
						break;
				}
			}
		}
	}

	void notify_resize(const int width, const int height)
	{
		queue_dimension.try_push(width, height);
	}

	void notify_input(const engine::hid::Input & input)
	{
		const auto res = queue_input.try_push(input);
		debug_assert(res);
	}

	void post_add_context(
		engine::Asset context)
	{
		const auto res = queue_add_context.try_emplace(context);
		debug_assert(res);
	}
	void post_remove_context(
		engine::Asset context)
	{
		const auto res = queue_remove_context.try_emplace(context);
		debug_assert(res);
	}
	void post_activate_context(
		engine::Asset context)
	{
		const auto res = queue_activate_context.try_emplace(context);
		debug_assert(res);
	}

	void post_add_contextswitch(
		engine::Entity entity,
		engine::hid::Input::Button button,
		engine::Asset context)
	{
		const auto res = queue_add_contextswitch.try_emplace(entity, button, context);
		debug_assert(res);
	}
	void post_add_flycontrol(
		engine::Entity entity,
		engine::Entity callback)
	{
		const auto res = queue_add_flycontrol.try_emplace(entity, callback);
		debug_assert(res);
	}
	void post_add_pancontrol(
		engine::Entity entity,
		engine::Entity callback)
	{
		const auto res = queue_add_pancontrol.try_emplace(entity, callback);
		debug_assert(res);
	}
	void post_add_renderhover(
		engine::Entity entity)
	{
		const auto res = queue_add_renderhover.try_emplace(entity, 0);
		debug_assert(res);
	}
	void post_add_renderselect(
		engine::Entity entity)
	{
		const auto res = queue_add_renderselect.try_emplace(entity, 0);
		debug_assert(res);
	}
	void post_add_renderswitch(
		engine::Entity entity,
		engine::hid::Input::Button button)
	{
		const auto res = queue_add_renderswitch.try_emplace(entity, button);
		debug_assert(res);
	}
	void post_remove(
		engine::Entity entity)
	{
		const auto res = queue_remove.try_emplace(entity, 0);
		debug_assert(res);
	}

	void post_bind(
		engine::Asset context,
		engine::Entity entity,
		int priority)
	{
		const auto res = queue_bind.try_emplace(context, entity, priority);
		debug_assert(res);
	}
	void post_unbind(
		engine::Asset context,
		engine::Entity entity)
	{
		const auto res = queue_unbind.try_emplace(context, entity);
		debug_assert(res);
	}
}
}
