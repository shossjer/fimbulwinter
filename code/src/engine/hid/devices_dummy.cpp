#include "config.h"

#if WINDOW_USE_DUMMY

#include "engine/hid/devices.hpp"

namespace engine
{
	namespace hid
	{
		void destroy_subsystem(devices &)
		{
		}

		void create_subsystem(devices &, engine::application::window & /*window*/, bool /*hardware_input*/)
		{
		}
	}
}

#endif
