
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

	using Device = int;
	std::vector<Device> devices;

	int find_device(Device device)
	{
		auto it = std::find(devices.begin(), devices.end(), device);
		debug_assert(it != devices.end());

		return std::distance(devices.begin(), it);
	}

	void add_device(Device device)
	{
		debug_assert(std::find(devices.begin(), devices.end(), device) == devices.end());

		devices.push_back(device);
	}

	void remove_device(Device device)
	{
		const int i = find_device(device);
		devices.erase(std::next(devices.begin(), i));
	}

	struct Context
	{
		std::vector<engine::Asset> states;

		std::vector<Device> devices;

		Context(std::vector<engine::Asset> && states)
			: states(std::move(states))
		{
			debug_assert(!this->states.empty(), "a context without states is useless, and a special case we do not want to handle");
		}
		void add_device(Device device)
		{
			debug_assert(std::find(::devices.begin(), ::devices.end(), device) != ::devices.end());
			debug_assert(std::find(devices.begin(), devices.end(), device) == devices.end());

			devices.push_back(device);
		}

		void remove_device(Device device)
		{
			debug_assert(std::find(::devices.begin(), ::devices.end(), device) != ::devices.end());

			auto it = std::find(devices.begin(), devices.end(), device);
			debug_assert(it != devices.end());

			devices.erase(it);
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

	struct AddDevice
	{
		engine::Asset context;
		int id;
	};

	struct RemoveContext
	{
		engine::Asset context;
	};

	struct RemoveDevice
	{
		engine::Asset context;
		int id;
	};

	using Message = utility::variant
	<
		AddContext,
		AddDevice,
		RemoveContext,
		RemoveDevice,
	>;

	core::container::CircleQueueSRMW<Message, 100> queue;

	void post_add_device(
		engine::Asset context,
		int id)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddDevice>, context, id);
		debug_assert(res);
	}
	void post_remove_device(
		engine::Asset context,
		int id)
	{
		const auto res = queue.try_emplace(utility::in_place_type<RemoveDevice>, context, id);
		debug_assert(res);
	}

	core::container::ExchangeQueueSRSW<dimension_t> queue_dimension;

	struct DeviceFound
	{
		int id;
	};

	struct DeviceLost
	{
		int id;
	};

	using InputMessage = utility::variant
	<
		DeviceFound,
		DeviceLost,
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

				void operator () (AddDevice && x)
				{
					const int i = find_context(x.context);
					contexts[i].add_device(x.id);
				}

				void operator () (RemoveContext && x)
				{
					remove_context(x.context);
				}

				void operator () (RemoveDevice && x)
				{
					const int i = find_context(x.context);
					contexts[i].remove_device(x.id);
				}

			} visitor;
			visit(visitor, std::move(message));
		}

		InputMessage message_input;
		while (queue_input.try_pop(message_input))
		{
			struct
			{

				void operator () (DeviceFound && x)
				{
					add_device(x.id);

					// v tmp v
					for (auto context : context_assets)
					{
						post_add_device(context, x.id);
					}
					// ^ tmp ^
				}

				void operator () (DeviceLost && x)
				{
					// v tmp v
					for (auto context : context_assets)
					{
						post_remove_device(context, x.id);
					}
					// ^ tmp ^

					remove_device(x.id);
				}

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

	void notify_device_found(int id)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<DeviceFound>, id);
		debug_assert(res);
	}

	void notify_device_lost(int id)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<DeviceLost>, id);
		debug_assert(res);
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
