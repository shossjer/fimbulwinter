
#include "input.hpp"

namespace gameplay
{
	namespace ui
	{
		extern void notify_input(const engine::hid::Input & input);
	}
}

namespace engine
{
	namespace hid
	{
		void dispatch(const Input & input)
		{
			gameplay::ui::notify_input(input);
		}
	}
}
