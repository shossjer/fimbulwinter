
#include "input.hpp"

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

namespace engine
{
	namespace hid
	{
		void dispatch(const Input & input)
		{
			ui::notify_input(input);
		}
	}
}
