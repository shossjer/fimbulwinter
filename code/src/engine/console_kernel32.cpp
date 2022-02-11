#include "config.h"

#if CONSOLE_USE_KERNEL32

#include "core/async/Thread.hpp"
#include "core/debug.hpp"

#include "engine/console.hpp"

#include "ful/convert.hpp"

#include <windows.h>

namespace engine
{
	namespace detail
	{
		void read_input(ful::view_utf8 line);
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
	core::async::thread_return thread_decl read_input(core::async::thread_param arg)
	{
		fiw_unused(arg);

#if 0
		// todo we would like to use this code but the windows console
		// cannot read utf8
		//
		// https://github.com/microsoft/terminal/issues/4551

		if (!debug_verify(::SetConsoleCP(65001) != FALSE, "failed with last error ", ::GetLastError())) // utf8
			return;

		while (active)
		{
			ful::unit_utf8 buffer[512]; // arbitrary
			DWORD chars_read = 0; // note must be initialized since canceling the read will return success without modifying it
			if (::ReadConsoleA(handle, buffer, sizeof buffer, &chars_read, nullptr) == FALSE)
			{
				debug_printline("ReadConsoleA failed with last error ", ::GetLastError());

				callback_exit();

				break;
			}
			if (chars_read == 0)
				break;
			if (!debug_assert(chars_read >= 2))
				break;

			engine::detail::read_input(ful::view_utf8(buffer + 0, chars_read - 2)); // two less to account for newline
		}
#else
		while (active)
		{
			ful::unit_utf8 buffer[512]; // arbitrary
			DWORD chars_read = 0; // note must be initialized since canceling the read will return success without modifying it
			ful::unit_utfw * const begw = reinterpret_cast<ful::unit_utfw *>(buffer + (sizeof buffer - (sizeof buffer / 3) * 2));
			if (::ReadConsoleW(handle, begw, sizeof buffer / 3, &chars_read, nullptr) == FALSE)
			{
				debug_printline("ReadConsoleW failed with last error ", ::GetLastError());

				callback_exit();

				break;
			}
			if (chars_read == 0)
				break;
			if (!debug_assert(chars_read >= 2))
				break;

			const ful::unit_utf8 * const end8 = ful::convert(begw, begw + chars_read - 2, buffer + 0); // two less to account for newline

			engine::detail::read_input(ful::view_utf8(buffer + 0, end8));
		}
#endif

		debug_printline("console thread stopping");

		return core::async::thread_return{};
	}
}

namespace engine
{
	console::~console()
	{
		active = false;
		CancelIoEx(handle, nullptr);

		thread.join();

		debug_verify(::CloseHandle(handle) != FALSE, "failed with last error ", ::GetLastError());

		::callback_exit = nullptr;
	}

	console::console(void (* callback_exit_)())
	{
		handle = ::CreateFileW(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (!debug_verify(handle != INVALID_HANDLE_VALUE, "failed with last error ", ::GetLastError()))
			return;

		::callback_exit = callback_exit_;

		active = true;
		thread = core::async::Thread(::read_input, nullptr);
	}
}

#endif
