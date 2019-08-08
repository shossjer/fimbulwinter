
#include "input.hpp"

#include <bitset>

namespace engine
{
	namespace hid
	{
		namespace ui
		{
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
