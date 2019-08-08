
#include "ui.hpp"

#include "core/container/CircleQueue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"

#include "engine/Command.hpp"
#include "engine/commands.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/hid/input.hpp"

#include "utility/variant.hpp"

#include <algorithm>
#include <tuple>
#include <vector>

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

	struct Context
	{
		std::vector<engine::Asset> states;
		Context(std::vector<engine::Asset> && states)
			: states(std::move(states))
		{
			debug_assert(!this->states.empty(), "a context without states is useless, and a special case we do not want to handle");
		}
	};

	std::vector<engine::Asset> context_assets;
	std::vector<Context> contexts;

	int find_context(engine::Asset context)
	{
		auto it = std::find(context_assets.begin(), context_assets.end(), context);
		debug_assert(it != context_assets.end());

		return std::distance(context_assets.begin(), it);
	}

	void add_context(engine::Asset context, std::vector<engine::Asset> && states)
	{
		debug_assert(std::find(context_assets.begin(), context_assets.end(), context) == context_assets.end());

		context_assets.push_back(context);
		contexts.emplace_back(std::move(states));
	}

	void remove_context(engine::Asset context)
	{
		const int i = find_context(context);
		contexts.erase(std::next(contexts.begin(), i));
		context_assets.erase(std::next(context_assets.begin(), i));
	}

	struct AddContext
	{
		engine::Asset context;
		std::vector<engine::Asset> states;
	};

	struct RemoveContext
	{
		engine::Asset context;
	};

	using Message = utility::variant
	<
		AddContext,
		RemoveContext,
	>;

	core::container::CircleQueueSRMW<Message, 100> queue;

	core::container::ExchangeQueueSRSW<dimension_t> queue_dimension;
	using InputMessage = utility::variant
	<
		engine::hid::Input
	>;

	core::container::CircleQueueSRMW<InputMessage, 500> queue_input;
}

namespace engine
{
namespace hid
{
namespace ui
{
	void create()
	{
	}

	void destroy()
	{
	}

	void update()
	{
		dimension_t notification_dimension;
		if (queue_dimension.try_pop(notification_dimension))
		{
			dimension = notification_dimension;
		}

		Message message;
		while (queue.try_pop(message))
		{
			struct
			{

				void operator () (AddContext && x)
				{
					add_context(x.context, std::move(x.states));
				}
				void operator () (RemoveContext && x)
				{
					remove_context(x.context);
				}


			} visitor;
			visit(visitor, std::move(message));
		}

		InputMessage message_input;
		while (queue_input.try_pop(message_input))
		{
			struct
			{

				void operator () (engine::hid::Input && input)
				{
				}

			} visitor;
			visit(visitor, std::move(message_input));
		}
	}

	void notify_resize(const int width, const int height)
	{
		queue_dimension.try_push(width, height);
	}

	void notify_input(const engine::hid::Input & input)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<engine::hid::Input>, input);
		debug_assert(res);
	}

	void post_add_context(
		engine::Asset context,
		std::vector<engine::Asset> states)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddContext>, context, std::move(states));
		debug_assert(res);
	}
	void post_remove_context(
		engine::Asset context)
	{
		const auto res = queue.try_emplace(utility::in_place_type<RemoveContext>, context);
		debug_assert(res);
	}
}
}
}
