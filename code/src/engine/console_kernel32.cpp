
#include "config.h"

#if CONSOLE_USE_KERNEL32

#include "console.hpp"

#include "core/async/Thread.hpp"
#include "core/debug.hpp"

#include "engine/application/window.hpp"

#include <iostream>
#include <string>

#include <windows.h>

namespace engine
{
	namespace console
	{
		void read_input(std::string line);
	}
}

namespace
{
	volatile bool active;
	core::async::Thread thread;

	HANDLE handle = nullptr;
}

namespace
{
	void read_input()
	{
		while (active)
		{
			std::string line;
			if (!std::getline(std::cin, line))
			{
				engine::application::window::close();
				break;
			}

			engine::console::read_input(line);
		}

		debug_printline("console thread stopping");
	}
}

namespace engine
{
	namespace console
	{
		void create()
		{
			active = true;
			handle = GetStdHandle(STD_INPUT_HANDLE);

			thread = core::async::Thread{ ::read_input };
		}

		void destroy()
		{
			active = false;
			CancelIoEx(handle, NULL);

			thread.join();
		}
	}
}

#endif
