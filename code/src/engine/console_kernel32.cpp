#include "config.h"

#if CONSOLE_USE_KERNEL32

#include "console.hpp"

#include "core/async/Thread.hpp"
#include "core/debug.hpp"

#include "utility/unicode/string_view.hpp"

#include <iostream>
#include <string>

#include <windows.h>

namespace engine
{
	namespace detail
	{
		void read_input(utility::string_units_utf8 line);
	}
}

namespace
{
	volatile bool active;
	core::async::Thread thread;

	HANDLE handle = nullptr;

	void (* callback_exit)() = nullptr;
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
				callback_exit();
				break;
			}

			engine::detail::read_input(utility::string_units_utf8(line.data(), line.size()));
		}

		debug_printline("console thread stopping");
	}
}

namespace engine
{
	console::~console()
	{
		active = false;
		CancelIoEx(handle, NULL);

		thread.join();

		::callback_exit = nullptr;
	}

	console::console(void (* callback_exit_)())
	{
		::callback_exit = callback_exit_;

		active = true;
		handle = GetStdHandle(STD_INPUT_HANDLE);

		thread = core::async::Thread(::read_input);
	}
}

#endif
