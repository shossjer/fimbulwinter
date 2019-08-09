
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

			extern void notify_input(const engine::hid::Input & input);
		}
	}
}

namespace
{
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

		void dispatch(const Input & input)
		{
			switch (input.getState())
			{
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
