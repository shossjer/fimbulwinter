
#include "devices.hpp"

#include "engine/hid/input.hpp"

#include <bitset>
#include <vector>

namespace engine
{
	namespace hid
	{
		extern void notify_device_found(ui & ui, int id);
		extern void notify_device_lost(ui & ui, int id);

		extern void notify_add_source(ui & ui, int id, std::string && path, int type, std::string && name);
		extern void notify_remove_source(ui & ui, int id, std::string && path);

		extern void notify_input(ui & ui, const engine::hid::Input & input);

		extern void destroy_subsystem(devices & devices);
		extern void create_subsystem(devices & devices, engine::application::window & window, bool hardware_input);
	}
}

namespace
{
	engine::hid::ui * ui = nullptr;

	struct DeviceValues
	{
		union axis_value
		{
			int32_t signed_value;
			uint32_t unsigned_value;
		};

		axis_value axis_states[engine::hid::Input::axis_count] = {};
		std::bitset<engine::hid::Input::button_count> button_states;
	};

	std::vector<DeviceValues> device_values;
}

namespace engine
{
	namespace hid
	{
		devices::~devices()
		{
			destroy_subsystem(*this);

			::ui = nullptr;
		}

		devices::devices(engine::application::window & window, engine::hid::ui & ui, bool hardware_input)
		{
			::ui = &ui;

			create_subsystem(*this, window, hardware_input);
		}

		void found_device(int id, int /*vendor*/, int /*product*/)
		{
			if (std::size_t(id) >= device_values.size())
			{
				device_values.resize(id + 1);
			}

			notify_device_found(*::ui, id);
		}
		void lost_device(int id)
		{
			if (debug_assert(std::size_t(id) < device_values.size()))
			{
				device_values[id] = DeviceValues{};
			}

			notify_device_lost(*::ui, id);
		}

		void add_source(int id, const char * path, int type, const char * name)
		{
			notify_add_source(*::ui, id, path, type, name);
		}
		void remove_source(int id, const char * path)
		{
			notify_remove_source(*::ui, id, path);
		}

		void dispatch(const Input & input)
		{
			auto & values = device_values[debug_assert(std::size_t(input.getDevice()) < device_values.size()) ? input.getDevice() : 0];

			switch (input.getState())
			{
			case Input::State::AXIS_TILT:
				if (values.axis_states[static_cast<int>(input.getAxis())].signed_value == input.getTilt())
					return;

				values.axis_states[static_cast<int>(input.getAxis())].signed_value = input.getTilt();
				break;
			case Input::State::AXIS_TRIGGER:
				if (values.axis_states[static_cast<int>(input.getAxis())].unsigned_value == input.getTrigger())
					return;

				values.axis_states[static_cast<int>(input.getAxis())].unsigned_value = input.getTrigger();
				break;
			case Input::State::BUTTON_DOWN:
				if (values.button_states[static_cast<int>(input.getButton())])
					return;

				values.button_states.set(static_cast<int>(input.getButton()));
				break;
			case Input::State::BUTTON_UP:
				if (!values.button_states[static_cast<int>(input.getButton())])
					return;

				values.button_states.reset(static_cast<int>(input.getButton()));
				break;
			default:
				break;
			}

			notify_input(*::ui, input);
		}
	}
}
