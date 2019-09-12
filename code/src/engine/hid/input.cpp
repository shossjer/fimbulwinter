
#include "input.hpp"

#include <bitset>

namespace engine
{
	namespace hid
	{
		namespace ui
		{
			extern void notify_device_found(int id);
			extern void notify_device_lost(int id);

			extern void notify_add_source(int id, std::string && path, int type, std::string && name);
			extern void notify_remove_source(int id, std::string && path);

			extern void notify_input(const engine::hid::Input & input);
		}
	}
}

namespace
{
	union axis_value
	{
		int32_t signed_value;
		uint32_t unsigned_value;
	};

	axis_value axis_states[engine::hid::Input::axis_count] = {};
	std::bitset<engine::hid::Input::button_count> button_states;
}

namespace engine
{
	namespace hid
	{
		void found_device(int id)
		{
			ui::notify_device_found(id);
		}
		void lost_device(int id)
		{
			ui::notify_device_lost(id);
		}

		void add_source(int id, const char * path, int type, const char * name)
		{
			ui::notify_add_source(id, path, type, name);
		}
		void remove_source(int id, const char * path)
		{
			ui::notify_remove_source(id, path);
		}

		void dispatch(const Input & input)
		{
			switch (input.getState())
			{
			case Input::State::AXIS_TILT:
				if (axis_states[static_cast<int>(input.getAxis())].signed_value == input.getTilt())
					return;

				axis_states[static_cast<int>(input.getAxis())].signed_value = input.getTilt();
				break;
			case Input::State::AXIS_TRIGGER:
				if (axis_states[static_cast<int>(input.getAxis())].unsigned_value == input.getTrigger())
					return;

				axis_states[static_cast<int>(input.getAxis())].unsigned_value = input.getTrigger();
				break;
			case Input::State::BUTTON_DOWN:
				if (button_states[static_cast<int>(input.getButton())])
					return;

				button_states.set(static_cast<int>(input.getButton()));
				break;
			case Input::State::BUTTON_UP:
				if (!button_states[static_cast<int>(input.getButton())])
					return;

				button_states.reset(static_cast<int>(input.getButton()));
				break;
			default:
				break;
			}

			ui::notify_input(input);
		}
	}
}
