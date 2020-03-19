#include "config.h"

#if CONSOLE_USE_POSIX

#include "console.hpp"

#include "core/async/Thread.hpp"
#include "core/debug.hpp"

#include "utility/unicode.hpp"

#include <iostream>
#include <string>

#include <poll.h>
#include <unistd.h>

namespace engine
{
	namespace detail
	{
		void read_input(utility::string_view_utf8 line);
	}
}

namespace
{
	core::async::Thread thread;

	int interupt_pipe[2];

	void (* callback_exit)() = nullptr;
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
			const auto ret = poll(fds, 2, -1);
			if (ret == -1)
			{
				if (!debug_verify(errno == EINTR, "poll failed with ", errno))
					break;

				continue;
			}

			if (fds[0].revents & POLLHUP)
				break;

			if (fds[1].revents & POLLIN)
			{
				std::string line;
				if (!std::getline(std::cin, line))
				{
					callback_exit();
					break;
				}

				engine::detail::read_input(utility::string_view_utf8(line.data(), utility::unit_difference(line.size())));
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

		::callback_exit = nullptr;
	}

	console::console(void (* callback_exit)())
	{
		::callback_exit = callback_exit;

		pipe(interupt_pipe);

		thread = core::async::Thread(::read_input);
	}
}

#endif
