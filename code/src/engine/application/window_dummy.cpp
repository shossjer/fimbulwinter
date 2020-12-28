#include "config.h"

#if WINDOW_USE_DUMMY

#include "engine/application/window.hpp"

namespace engine
{
	namespace application
	{
		window::~window()
		{
		}

		window::window(const config_t & /*config*/)
		{
		}

		void window::set_dependencies(engine::graphics::viewer & /*viewer*/, engine::hid::devices & /*devices*/, engine::hid::ui & /*ui*/)
		{
		}

		void make_current(window &)
		{
		}

		void swap_buffers(window &)
		{
		}

		int execute(window &)
		{
			return 0;
		}

		void close()
		{
		}
	}
}

#endif
