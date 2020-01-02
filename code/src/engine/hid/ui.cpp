
#include "ui.hpp"

#include "core/async/delay.hpp"
#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"

#include "engine/Command.hpp"
#include "engine/commands.hpp"
#include "engine/console.hpp"
#include "engine/debug.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/hid/input.hpp"

#include "utility/any.hpp"
#include "utility/ranges.hpp"
#include "utility/variant.hpp"

#include <algorithm>
#include <tuple>
#include <vector>

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
		Filter axis_filters[engine::hid::Input::axis_count] = {};

		void (* button_callbacks[engine::hid::Input::button_count])(engine::Command command, float value, void * data);
		void (* axis_callbacks[engine::hid::Input::axis_count])(engine::Command command, float value, void * data);

		void * button_datas[engine::hid::Input::button_count];
		void * axis_datas[engine::hid::Input::axis_count];
	};

	struct DeviceSource
	{
		int type;
		std::string path;
		std::string name;
	};

	std::vector<Device> devices;
	std::vector<DeviceMapping> device_mappings;
	std::vector<std::vector<DeviceSource>> device_sources;

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
		device_sources.emplace_back();
	}

	void remove_device(Device device)
	{
		const int i = find_device(device);

		device_sources.erase(std::next(device_sources.begin(), i));
		device_mappings.erase(std::next(device_mappings.begin(), i));
		devices.erase(std::next(devices.begin(), i));
	}

	struct AxisMove
	{
		engine::Command command_x;
		engine::Command command_y;
	};

	struct AxisTilt
	{
		engine::Command command_min;
		engine::Command command_max;
	};

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
		utility::heap_storage<AxisMove>,
		utility::heap_storage<AxisTilt>,
		utility::heap_storage<ButtonPress>,
		utility::heap_storage<ButtonRelease>
	>
	filters;

	Filter next_available_filter = 1;

	struct Mapping
	{
		Filter buttons[engine::hid::Input::button_count] = {};
		Filter axes[engine::hid::Input::axis_count] = {};
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

	struct Context
	{
		struct StateMapping
		{
			std::vector<engine::Entity> mappings;
			std::vector<void (*)(engine::Command command, float value, void * data)> callbacks;
			std::vector<void *> datas;
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
		void add_mapping(engine::Asset state, engine::Entity mapping, void (* callback)(engine::Command command, float value, void * data), void * data)
		{
			debug_assert((!has_device_mappings || state != states[active]));

			const int i = find(state);
			debug_assert(std::find(state_mappings[i].mappings.begin(), state_mappings[i].mappings.end(), mapping) == state_mappings[i].mappings.end());

			state_mappings[i].mappings.push_back(mapping);
			state_mappings[i].callbacks.push_back(callback);
			state_mappings[i].datas.push_back(data);
		}

		// \note if `state` is the active state, the device mappings must be cleared prior to this call
		void remove_mapping(engine::Asset state, engine::Entity mapping)
		{
			debug_assert((!has_device_mappings || state != states[active]));

			const int i = find(state);
			auto it = std::find(state_mappings[i].mappings.begin(), state_mappings[i].mappings.end(), mapping);
			debug_assert(it != state_mappings[i].mappings.end());

			state_mappings[i].datas.erase(std::next(state_mappings[i].datas.begin(), std::distance(state_mappings[i].mappings.begin(), it)));
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

		bool has_device(Device device)
		{
			debug_assert(std::find(::devices.begin(), ::devices.end(), device) != ::devices.end());

			return std::find(devices.begin(), devices.end(), device) != devices.end();
		}

		// \note the device mappings must be cleared prior to this call
		void set_active(engine::Asset state)
		{
			debug_assert(!has_device_mappings);

			active = find(state);
		}

		void set_device_mappings()
		{
			debug_assert(!has_device_mappings, "already set");
			has_device_mappings = true;

			const auto & state_mapping = state_mappings[active];

			for (auto device : devices)
			{
				auto & device_mapping = device_mappings[find_device(device)];

				for (int j : ranges::index_sequence_for(state_mapping.mappings))
				{
					const auto & mapping = ::mappings[find_mapping(state_mapping.mappings[j])];

					for (int i : ranges::index_sequence(sizeof mapping.buttons / sizeof mapping.buttons[0]))
					{
						if (mapping.buttons[i])
						{
							debug_assert(device_mapping.button_filters[i] == Filter{}, "mapping conflict");

							device_mapping.button_filters[i] = mapping.buttons[i];
							device_mapping.button_callbacks[i] = state_mapping.callbacks[j];
							device_mapping.button_datas[i] = state_mapping.datas[j];
						}
					}

					for (int i : ranges::index_sequence(sizeof mapping.axes / sizeof mapping.axes[0]))
					{
						if (mapping.axes[i])
						{
							debug_assert(device_mapping.axis_filters[i] == Filter{}, "mapping conflict");

							device_mapping.axis_filters[i] = mapping.axes[i];
							device_mapping.axis_callbacks[i] = state_mapping.callbacks[j];
							device_mapping.axis_datas[i] = state_mapping.datas[j];
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

				for (int j : ranges::index_sequence_for(state_mapping.mappings))
				{
					const auto & mapping = ::mappings[find_mapping(state_mapping.mappings[j])];

					for (int i : ranges::index_sequence(sizeof mapping.buttons / sizeof mapping.buttons[0]))
					{
						if (mapping.buttons[i])
						{
							debug_assert(device_mapping.button_filters[i] == mapping.buttons[i], "sanity check");

							device_mapping.button_filters[i] = Filter{};
						}
					}

					for (int i : ranges::index_sequence(sizeof mapping.axes / sizeof mapping.axes[0]))
					{
						if (mapping.axes[i])
						{
							debug_assert(device_mapping.axis_filters[i] == mapping.axes[i], "sanity check");

							device_mapping.axis_filters[i] = Filter{};
						}
					}
				}
			}
		}
	};

	std::vector<engine::Asset> context_assets;
	std::vector<Context> contexts;

	engine::Asset default_context = engine::Asset::null();

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
		debug_assert(context != default_context);

		const int i = find_context(context);
		contexts.erase(std::next(contexts.begin(), i));
		context_assets.erase(std::next(context_assets.begin(), i));
	}

	struct AddAxisMove
	{
		engine::Entity mapping;
		engine::hid::Input::Axis code;
		engine::Command command_x;
		engine::Command command_y;
	};

	struct AddAxisTilt
	{
		engine::Entity mapping;
		engine::hid::Input::Axis code;
		engine::Command command_min;
		engine::Command command_max;
	};

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
		void (* callback)(engine::Command command, float value, void * data);
		void * data;
	};

	struct ContextInfo
	{
		engine::Asset asset;
		std::vector<engine::Asset> states;
		std::vector<int> devices;
	};
	struct QueryContexts
	{
		std::atomic_int * ready;
		std::vector<ContextInfo> * contexts;
	};

	struct DeviceInfo
	{
		int id;
		std::vector<DeviceSource> sources;
	};
	struct QueryDevices
	{
		std::atomic_int * ready;
		std::vector<DeviceInfo> * devices;
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

	struct SetState
	{
		engine::Asset context;
		engine::Asset state;
	};

	struct Unbind
	{
		engine::Asset context;
		engine::Asset state;
		engine::Entity mapping;
	};

	using Message = utility::variant
	<
		AddAxisMove,
		AddAxisTilt,
		AddButtonPress,
		AddButtonRelease,
		AddContext,
		AddDevice,
		Bind,
		QueryContexts,
		QueryDevices,
		RemoveContext,
		RemoveDevice,
		SetState,
		Unbind
	>;

	core::container::PageQueue<utility::heap_storage<Message>> queue;

	core::container::ExchangeQueueSRSW<dimension_t> queue_dimension;

	struct DeviceFound
	{
		int id;
	};

	struct DeviceLost
	{
		int id;
	};

	struct AddSource
	{
		int id;
		int type;
		std::string path;
		std::string name;
	};

	struct RemoveSource
	{
		int id;
		std::string path;
	};

	using InputMessage = utility::variant
	<
		DeviceFound,
		DeviceLost,
		AddSource,
		RemoveSource,
		engine::hid::Input
	>;

	core::container::PageQueue<utility::heap_storage<InputMessage>> queue_input;

	struct HandleAxisMove
	{
		void (* callback)(engine::Command command, float value, void * data);
		void * data;

		const engine::hid::Input & input;

		void operator () (utility::monostate)
		{
			// this device axis is not bound to any filter
			debug_printline(engine::hid_channel, "no filter for axis move");
		}

		void operator () (const AxisMove & x)
		{
			callback(x.command_x, static_cast<float>(input.getPosition().x), data);
			callback(x.command_y, static_cast<float>(input.getPosition().y), data);
		}

		template <typename T>
		void operator () (const T &)
		{
			debug_unreachable("unknown filter");
		}

	};

	struct HandleAxisTilt
	{
		void (* callback)(engine::Command command, float value, void * data);
		void * data;

		const engine::hid::Input & input;

		void operator () (utility::monostate)
		{
			// this device axis is not bound to any filter
			debug_printline(engine::hid_channel, "no filter for ", core::value_table<engine::hid::Input::Axis>::get_key(input.getAxis()));
		}

		void operator () (const AxisTilt & x)
		{
			const int value_min = -std::min(input.getTilt(), 0);
			const int value_max =  std::max(input.getTilt(), 0);
			const float real_min = static_cast<float>(static_cast<double>(value_min) / static_cast<double>((uint32_t(1) << 31) - 1));
			const float real_max = static_cast<float>(static_cast<double>(value_max) / static_cast<double>((uint32_t(1) << 31) - 1));
			callback(x.command_min, real_min, data);
			callback(x.command_max, real_max, data);
		}

		template <typename T>
		void operator () (const T &)
		{
			debug_unreachable("unknown filter");
		}

	};

	template <bool Reversed>
	struct HandleButton
	{
		void (* callback)(engine::Command command, float value, void * data);
		void * data;

		const engine::hid::Input & input;

		void operator () (utility::monostate)
		{
			// this device button is not bound to any filter
			debug_printline(engine::hid_channel, "no filter for ", core::value_table<engine::hid::Input::Button>::get_key(input.getButton()));
		}

		void operator () (const ButtonPress & x)
		{
			callback(x.command, Reversed ? 0.f : 1.f, data);
		}

		void operator () (const ButtonRelease & x)
		{
			callback(x.command, Reversed ? 1.f : 0.f, data);
		}

		template <typename T>
		void operator () (const T &)
		{
			debug_unreachable("unknown filter");
		}

	};

	void post_query_contexts(std::atomic_int & ready, std::vector<ContextInfo> & contexts_)
	{
		const auto res = queue.try_emplace(utility::in_place_type<QueryContexts>, &ready, &contexts_);
		debug_assert(res);
	}

	void post_query_devices(std::atomic_int & ready, std::vector<DeviceInfo> & devices_)
	{
		const auto res = queue.try_emplace(utility::in_place_type<QueryDevices>, &ready, &devices_);
		debug_assert(res);
	}

	void callback_print_devices(void *)
	{
		std::atomic_int ready(0);
		std::vector<DeviceInfo> devices_;
		std::vector<ContextInfo> contexts_;
		post_query_devices(ready, devices_);
		post_query_contexts(ready, contexts_);

		while (ready.load(std::memory_order_acquire) < 2)
		{
			core::async::yield();
		}

		debug_printline("print-devices:");
		for (const auto & device : devices_)
		{
			std::vector<int> contexts_using_device;
			for (int i : ranges::index_sequence_for(contexts_))
			{
				if (std::find(contexts_[i].devices.begin(), contexts_[i].devices.end(), device.id) != contexts_[i].devices.end())
				{
					contexts_using_device.push_back(i);
				}
			}

			std::string context_str;
			if (contexts_using_device.empty())
			{
				context_str = " unsused";
			}
			else
			{
				context_str = " used in contexts:";
				for (auto i : contexts_using_device)
				{
					context_str += utility::to_string(" ", contexts_[i].asset);
				}
			}
			debug_printline(" device ", device.id, context_str);

			for (const auto & source : device.sources)
			{
				debug_printline("  source ", source.path, "(", source.type, "): ", source.name);
			}
		}
	}
}

namespace engine
{
namespace hid
{
	ui::~ui()
	{
		engine::abandon("print-devices");
	}

	ui::ui()
	{
		engine::observe("print-devices", callback_print_devices, nullptr);
	}

	void update(ui &)
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

				void operator () (AddAxisMove && x)
				{
					const Filter filter = next_available_filter++;
					debug_verify(filters.try_emplace<AxisMove>(filter, x.command_x, x.command_y));

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.axes[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.axes[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddAxisTilt && x)
				{
					const Filter filter = next_available_filter++;
					debug_verify(filters.try_emplace<AxisTilt>(filter, x.command_min, x.command_max));

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.axes[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.axes[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddButtonPress && x)
				{
					const Filter filter = next_available_filter++;
					debug_verify(filters.try_emplace<ButtonPress>(filter, x.command));

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.buttons[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.buttons[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddButtonRelease && x)
				{
					const Filter filter = next_available_filter++;
					debug_verify(filters.try_emplace<ButtonRelease>(filter, x.command));

					auto & mapping = mappings[add_or_find_mapping(x.mapping)];
					debug_assert(mapping.buttons[static_cast<int>(x.code)] == Filter{}, "mapping contains conflicts");
					mapping.buttons[static_cast<int>(x.code)] = filter;
				}

				void operator () (AddContext && x)
				{
					const bool first_context = context_assets.empty();

					add_context(x.context, std::move(x.states));

					if (first_context)
					{
						// it is a bit weird that we make the first context
						// the default context...
						default_context = x.context;

						// ... and after that add all known devices to that
						// context, it is confusing to have multiple
						// meanings for what a default context is, therefore
						// we should probably replace this loop with
						// something else, but what? :thinking:

						// (the primary meaning being of course that found
						// devices gets added to the default context if one
						// is set, see DeviceFound)
						const int i = find_context(default_context);
						debug_assert(!contexts[i].has_device_mappings, "newly added context should not have device mappings");
						for (auto device : devices)
						{
							contexts[i].add_device(device);
						}
					}
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

					contexts[i].add_mapping(x.state, x.mapping, x.callback, x.data);
				}

				void operator () (QueryContexts && x)
				{
					x.contexts->resize(contexts.size());

					for (int i : ranges::index_sequence_for(contexts))
					{
						(*x.contexts)[i].asset = context_assets[i];
						(*x.contexts)[i].devices = contexts[i].devices;
						(*x.contexts)[i].states = contexts[i].states;
					}

					x.ready->fetch_add(1, std::memory_order_release);
				}

				void operator () (QueryDevices && x)
				{
					x.devices->resize(devices.size());

					for (int i : ranges::index_sequence_for(devices))
					{
						(*x.devices)[i].id = devices[i];
						(*x.devices)[i].sources = device_sources[i];
					}

					x.ready->fetch_add(1, std::memory_order_release);
				}

				void operator () (RemoveContext && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings)
					{
						contexts[i].clear_device_mappings();
					}

					const bool removing_default_context = x.context == default_context;
					if (removing_default_context)
					{
						default_context = engine::Asset::null();
					}

					remove_context(x.context);

					if (removing_default_context)
					{
						if (!context_assets.empty())
						{
							default_context = context_assets.front();
						}
					}
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

				void operator () (SetState && x)
				{
					const int i = find_context(x.context);
					if (contexts[i].has_device_mappings)
					{
						contexts[i].clear_device_mappings();
					}

					contexts[i].set_active(x.state);
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
					debug_printline("device ", x.id, " found");

					add_device(x.id);

					if (default_context != engine::Asset::null())
					{
						const int i = find_context(default_context);
						contexts[i].clear_device_mappings();
						contexts[i].add_device(x.id);
						contexts[i].set_device_mappings();
					}
				}

				void operator () (DeviceLost && x)
				{
					debug_printline("device ", x.id, " lost");

					for (auto & context : contexts)
					{
						if (!context.has_device(x.id))
							continue;

						context.clear_device_mappings();
						context.remove_device(x.id);
						context.set_device_mappings();
					}

					remove_device(x.id);
				}

				void operator () (AddSource && x)
				{
					debug_printline("device ", x.id, " found source ", x.path);

					const int i = find_device(x.id);
					debug_assert(std::find_if(device_sources[i].begin(), device_sources[i].end(), [&](const DeviceSource & source){ return source.path == x.path; }) == device_sources[i].end());
					device_sources[i].push_back({x.type, std::move(x.path), std::move(x.name)});
				}

				void operator () (RemoveSource && x)
				{
					debug_printline("device ", x.id, " lost source ", x.path);

					const int i = find_device(x.id);
					auto source = std::find_if(device_sources[i].begin(), device_sources[i].end(), [&](const DeviceSource & source){ return source.path == x.path; });
					debug_assert(source != device_sources[i].end());
					device_sources[i].erase(source);
				}

				void operator () (engine::hid::Input && input)
				{
					const auto & device_mapping = device_mappings[find_device(input.getDevice())];

					switch (input.getState())
					{
					case engine::hid::Input::State::AXIS_TILT:
						filters.try_call(device_mapping.axis_filters[static_cast<int>(input.getAxis())], HandleAxisTilt{device_mapping.axis_callbacks[static_cast<int>(input.getAxis())], device_mapping.axis_datas[static_cast<int>(input.getAxis())], input});
						break;
					case engine::hid::Input::State::BUTTON_DOWN:
						filters.try_call(device_mapping.button_filters[static_cast<int>(input.getButton())], HandleButton<false>{device_mapping.button_callbacks[static_cast<int>(input.getButton())], device_mapping.button_datas[static_cast<int>(input.getButton())], input});
						break;
					case engine::hid::Input::State::BUTTON_UP:
						filters.try_call(device_mapping.button_filters[static_cast<int>(input.getButton())], HandleButton<true>{device_mapping.button_callbacks[static_cast<int>(input.getButton())], device_mapping.button_datas[static_cast<int>(input.getButton())], input});
						break;
					case engine::hid::Input::State::CURSOR_MOVE:
						filters.try_call(device_mapping.axis_filters[static_cast<int>(engine::hid::Input::Axis::MOUSE_MOVE)], HandleAxisMove{device_mapping.axis_callbacks[static_cast<int>(engine::hid::Input::Axis::MOUSE_MOVE)], device_mapping.axis_datas[static_cast<int>(engine::hid::Input::Axis::MOUSE_MOVE)], input});
						break;
					}
				}

			} visitor;
			visit(visitor, std::move(message_input));
		}
	}

	void notify_resize(ui &, const int width, const int height)
	{
		queue_dimension.try_push(width, height);
	}

	void notify_device_found(ui &, int id)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<DeviceFound>, id);
		debug_assert(res);
	}

	void notify_device_lost(ui &, int id)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<DeviceLost>, id);
		debug_assert(res);
	}

	void notify_add_source(ui &, int id, std::string && path, int type, std::string && name)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<AddSource>, id, type, std::move(path), std::move(name));
		debug_assert(res);
	}

	void notify_remove_source(ui &, int id, std::string && path)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<RemoveSource>, id, std::move(path));
		debug_assert(res);
	}

	void notify_input(ui &, const engine::hid::Input & input)
	{
		const auto res = queue_input.try_emplace(utility::in_place_type<engine::hid::Input>, input);
		debug_assert(res);
	}

	void post_add_context(
		ui &,
		engine::Asset context,
		std::vector<engine::Asset> states)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddContext>, context, std::move(states));
		debug_assert(res);
	}
	void post_remove_context(
		ui &,
		engine::Asset context)
	{
		const auto res = queue.try_emplace(utility::in_place_type<RemoveContext>, context);
		debug_assert(res);
	}

	void post_set_state(
		ui &,
		engine::Asset context,
		engine::Asset state)
	{
		const auto res = queue.try_emplace(utility::in_place_type<SetState>, context, state);
		debug_assert(res);
	}

	void post_add_device(
		ui &,
		engine::Asset context,
		int id)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddDevice>, context, id);
		debug_assert(res);
	}
	void post_remove_device(
		ui &,
		engine::Asset context,
		int id)
	{
		const auto res = queue.try_emplace(utility::in_place_type<RemoveDevice>, context, id);
		debug_assert(res);
	}

	void post_add_axis_move(
		ui &,
		engine::Entity mapping,
		engine::hid::Input::Axis code,
		engine::Command command_x,
		engine::Command command_y)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddAxisMove>, mapping, code, command_x, command_y);
		debug_assert(res);
	}
	void post_add_axis_tilt(
		ui &,
		engine::Entity mapping,
		engine::hid::Input::Axis code,
		engine::Command command_min,
		engine::Command command_max)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddAxisTilt>, mapping, code, command_min, command_max);
		debug_assert(res);
	}
	void post_add_button_press(
		ui &,
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddButtonPress>, mapping, code, command);
		debug_assert(res);
	}
	void post_add_button_release(
		ui &,
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command)
	{
		const auto res = queue.try_emplace(utility::in_place_type<AddButtonRelease>, mapping, code, command);
		debug_assert(res);
	}

	void post_bind(
		ui &,
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping,
		void (* callback)(engine::Command command, float value, void * data),
		void * data)
	{
		const auto res = queue.try_emplace(utility::in_place_type<Bind>, context, state, mapping, callback, data);
		debug_assert(res);
	}
	void post_unbind(
		ui &,
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping)
	{
		const auto res = queue.try_emplace(utility::in_place_type<Unbind>, context, state, mapping);
		debug_assert(res);
	}
}
}
