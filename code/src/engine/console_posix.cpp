#include "config.h"

#if CONSOLE_USE_POSIX

#include "console.hpp"

#include "core/async/Thread.hpp"
#include "core/debug.hpp"

#include "engine/application/window.hpp"

#include <iostream>
#include <string>

#include <poll.h>
#include <unistd.h>

namespace engine
{
	namespace application
	{
		extern void close(window & window);
	}

	namespace detail
	{
		void read_input(std::string line);
	}
}

namespace
{
	core::async::Thread thread;

	int interupt_pipe[2];

	engine::application::window * window = nullptr;
}

namespace
{
	void read_input()
	{
		struct pollfd fds[2] = {
			{interupt_pipe[0], 0, 0},
			{STDIN_FILENO, POLLIN, 0}
		};

		while (true)
		{
			const auto n = poll(fds, 2, -1);

			if (fds[0].revents & POLLHUP)
				break;

			if (fds[1].revents & POLLIN)
			{
				std::string line;
				if (!std::getline(std::cin, line))
				{
					close(*window);
					break;
				}

				engine::detail::read_input(line);
			}
		}
		close(interupt_pipe[0]);

		debug_printline("console thread stopping");
	}
}

namespace engine
{
	console::~console()
	{
		close(interupt_pipe[1]);

		thread.join();

		::window = nullptr;
	}

	console::console(engine::application::window & window)
	{
		::window = &window;

		pipe(interupt_pipe);

		thread = core::async::Thread(::read_input);
	}
}

#endif
