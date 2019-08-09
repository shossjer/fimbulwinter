
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
	using Filter = int;

	struct DeviceMapping
	{
		Filter button_filters[engine::hid::Input::button_count] = {};

		engine::Entity button_callbacks[engine::hid::Input::button_count];
	};

	std::vector<Device> devices;
	std::vector<DeviceMapping> device_mappings;

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
		device_mappings.emplace_back();
	}

	void remove_device(Device device)
	{
		const int i = find_device(device);

		device_mappings.erase(std::next(device_mappings.begin(), i));
		devices.erase(std::next(devices.begin(), i));
	}

	struct ButtonPress
	{
		engine::Command command;
	};

	struct ButtonRelease
	{
		engine::Command command;
	};

	core::container::Collection
	<
		Filter,
		201,
		utility::heap_storage<ButtonPress>,
		utility::heap_storage<ButtonRelease>
	>
	filters;

	Filter next_available_filter = 1;

	struct Mapping
	{
		Filter buttons[engine::hid::Input::button_count] = {};
	};

	std::vector<engine::Entity> mapping_entities;
	std::vector<Mapping> mappings;

	int find_mapping(engine::Entity mapping)
	{
		auto it = std::find(mapping_entities.begin(), mapping_entities.end(), mapping);
		debug_assert(it != mapping_entities.end());

		return std::distance(mapping_entities.begin(), it);
	}

	int add_or_find_mapping(engine::Entity mapping)
	{
		auto it = std::find(mapping_entities.begin(), mapping_entities.end(), mapping);
		const int i = std::distance(mapping_entities.begin(), it);
		if (it == mapping_entities.end())
		{
			mapping_entities.push_back(mapping);
			mappings.emplace_back();
		}
		return i;
	}

	void remove_mapping(engine::Entity mapping)
	{
		const int i = find_mapping(mapping);
		mappings.erase(std::next(mappings.begin(), i));
		mapping_entities.erase(std::next(mapping_entities.begin(), i));
	}

	struct Context
	{
		struct StateMapping
		{
			std::vector<engine::Entity> mappings;
			std::vector<engine::Entity> callbacks;
		};

		int active = 0;
		bool has_device_mappings = false;

		std::vector<engine::Asset> states;
		std::vector<StateMapping> state_mappings;

		std::vector<Device> devices;

		Context(std::vector<engine::Asset> && states)
			: states(std::move(states))
			, state_mappings(this->states.size())
		{
			debug_assert(!this->states.empty(), "a context without states is useless, and a special case we do not want to handle");
		}

		int find(engine::Asset state) const
		{
			auto it = std::find(states.begin(), states.end(), state);
			debug_assert(it != states.end());

			return std::distance(states.begin(), it);
		}

		// \note if `state` is the active state, the device mappings must be cleared prior to this call
		void add_mapping(engine::Asset state, engine::Entity mapping, engine::Entity callback)
		{
			debug_assert((!has_device_mappings || state != states[active]));

			const int i = find(state);
			debug_assert(std::find(state_mappings[i].mappings.begin(), state_mappings[i].mappings.end(), mapping) == state_mappings[i].mappings.end());

			state_mappings[i].mappings.push_back(mapping);
			state_mappings[i].callbacks.push_back(callback);
		}

		// \note if `state` is the active state, the device mappings must be cleared prior to this call
		void remove_mapping(engine::Asset state, engine::Entity mapping)
		{
			debug_assert((!has_device_mappings || state != states[active]));

			const int i = find(state);
			auto it = std::find(state_mappings[i].mappings.begin(), state_mappings[i].mappings.end(), mapping);
			debug_assert(it != state_mappings[i].mappings.end());

			state_mappings[i].callbacks.erase(std::next(state_mappings[i].callbacks.begin(), std::distance(state_mappings[i].mappings.begin(), it)));
			state_mappings[i].mappings.erase(it);
		}

		// \note the device mappings must be cleared prior to this call
		void add_device(Device device)
		{
			debug_assert(!has_device_mappings);
			debug_assert(std::find(::devices.begin(), ::devices.end(), device) != ::devices.end());
			debug_assert(std::find(devices.begin(), devices.end(), device) == devices.end());

			devices.push_back(device);
		}

		// \note the device mappings must be cleared prior to this call
		void remove_device(Device device)
		{
			debug_assert(!has_device_mappings);
			debug_assert(std::find(::devices.begin(), ::devices.end(), device) != ::devices.end());

			auto it = std::find(devices.begin(), devices.end(), device);
			debug_assert(it != devices.end());

			devices.erase(it);
		}

		void set_device_mappings()
		{
			debug_assert(!has_device_mappings, "already set");
			has_device_mappings = true;

			const auto & state_mapping = state_mappings[active];

			for (auto device : devices)
			{
				auto & device_mapping = device_mappings[find_device(device)];

				for (int j = 0; j < state_mapping.mappings.size(); j++)
				{
					const auto & mapping = ::mappings[find_mapping(state_mapping.mappings[j])];

					for (int i = 0; i < sizeof mapping.buttons / sizeof mapping.buttons[0]; i++)
					{
						if (mapping.buttons[i])
						{
							debug_assert(device_mapping.button_filters[i] == Filter{}, "mapping conflict");

							device_mapping.button_filters[i] = mapping.buttons[i];
							device_mapping.button_callbacks[i] = state_mapping.callbacks[j];
						}
					}
				}
			}
		}

		void clear_device_mappings()
		{
			debug_assert(has_device_mappings, "already cleared");
			has_device_mappings = false;

			const auto & state_mapping = state_mappings[active];

			for (auto device : devices)
			{
				auto & device_mapping = device_mappings[find_device(device)];

				for (int j = 0; j < state_mapping.mappings.size(); j++)
				{
					const auto & mapping = ::mappings[find_mapping(state_mapping.mappings[j])];

					for (int i = 0; i < sizeof mapping.buttons / sizeof mapping.buttons[0]; i++)
					{
						if (mapping.buttons[i])
						{
							debug_assert(device_mapping.button_filters[i] == mapping.buttons[i], "sanity check");

							device_mapping.button_filters[i] = Filter{};
						}
					}
				}
			}
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

	struct AddButtonPress
	{
		engine::Entity mapping;
		engine::hid::Input::Button code;
		engine::Command command;
	};

	struct AddButtonRelease
	{
		engine::Entity mapping;
		engine::hid::Input::Button code;
		engine::Command command;
	};

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

	struct Bind
	{
		engine::Asset context;
		engine::Asset state;
		engine::Entity mapping;
		engine::Entity callback;
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

	struct Unbind
	{
		engine::Asset context;
		engine::Asset state;
		engine::Entity mapping;
	};

	using Message = utility::variant
	<
		AddButtonPress,
		AddButtonRelease,
		AddContext,
		AddDevice,
		Bind,
		RemoveContext,
		RemoveDevice,
		Unbind
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

	template <bool Reversed>
	struct ButtonToggleFilter
	{
		engine::Entity callback;

		const engine::hid::Input & input;

		void operator () (utility::monostate)
		{
			// this device button is not bound to any filter
			debug_printline(engine::hid_channel, "no filter for ", core::value_table<engine::hid::Input::Button>::get_key(input.getButton()));
		}

		void operator () (const ButtonPress & x)
		{
		}

		void operator () (const ButtonRelease & x)
		{
		}

		template <typename T>
		void operator () (const T &)
		{
			debug_unreachable("unknown filter");
		}

	};
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

				void operator () (AddButtonPress && x)
				{
					const Filter filter = next_available_filter++;
					filters.emplace<ButtonPress>(filter, x.command);

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.buttons[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.buttons[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddButtonRelease && x)
				{
					const Filter filter = next_available_filter++;
					filters.emplace<ButtonRelease>(filter, x.command);

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.buttons[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.buttons[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddContext && x)
				{
					add_context(x.context, std::move(x.states));
				}

				void operator () (AddDevice && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings)
					{
						contexts[i].clear_device_mappings();
					}

					contexts[i].add_device(x.id);
				}

				void operator () (Bind && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings && contexts[i].states[contexts[i].active] == x.state)
					{
						contexts[i].clear_device_mappings();
					}

					contexts[i].add_mapping(x.state, x.mapping, x.callback);
				}

				void operator () (RemoveContext && x)
				{
					remove_context(x.context);
				}

				void operator () (RemoveDevice && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings)
					{
						contexts[i].clear_device_mappings();
					}

					contexts[i].remove_device(x.id);
				}

				void operator () (Unbind && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings && contexts[i].states[contexts[i].active] == x.state)
					{
						contexts[i].clear_device_mappings();
					}

					contexts[i].remove_mapping(x.state, x.mapping);
				}

			} visitor;
			visit(visitor, std::move(message));
		}

		for (auto & context : contexts)
		{
			if (!context.has_device_mappings)
			{
				context.set_device_mappings();
			}
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
					const auto & device_mapping = device_mappings[find_device(input.getDevice())];

					switch (input.getState())
					{
					case engine::hid::Input::State::BUTTON_DOWN:
						filters.try_call(device_mapping.button_filters[static_cast<int>(input.getButton())], ButtonToggleFilter<false>{device_mapping.button_callbacks[static_cast<int>(input.getButton())], input});
						break;
					case engine::hid::Input::State::BUTTON_UP:
						filters.try_call(device_mapping.button_filters[static_cast<int>(input.getButton())], ButtonToggleFilter<true>{device_mapping.button_callbacks[static_cast<int>(input.getButton())], input});
						break;
					}
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

	void post_add_button_press(
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddButtonPress>, mapping, code, command);
		debug_assert(res);
	}
	void post_add_button_release(
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddButtonRelease>, mapping, code, command);
		debug_assert(res);
	}

	void post_bind(
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping,
		engine::Entity callback)
	{
		const auto res = queue.try_emplace(utility::in_place_type<Bind>, context, state, mapping, callback);
		debug_assert(res);
	}
	void post_unbind(
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping)
	{
		const auto res = queue.try_emplace(utility::in_place_type<Unbind>, context, state, mapping);
		debug_assert(res);
	}
}
}
}
