#include "config.h"

#if THREAD_USE_KERNEL32

#include "Thread.hpp"

#include "core/debug.hpp"

#include <algorithm>

namespace
{
	DWORD WINAPI call_void(LPVOID arg)
	{
		((void(*)())arg)();

		return DWORD{0};
	}

	HANDLE create_thread(LPTHREAD_START_ROUTINE callback, LPVOID lpVoid)
	{
		HANDLE handle = CreateThread(nullptr, 0, callback, lpVoid, 0, nullptr);
		debug_assert(handle != nullptr, "CreateThread failed");
		return handle;
	}
}

namespace core
{
	namespace async
	{
		Thread::~Thread()
		{
			debug_assert(handle == nullptr, "thread might still be running");
		}

		Thread::Thread()
			: handle(nullptr)
		{}

		Thread::Thread(void (* callback)())
			: handle(create_thread(call_void, (LPVOID)callback))
		{}

		Thread::Thread(Thread && other)
			: handle(std::exchange(other.handle, nullptr))
		{}

		Thread & Thread::operator = (Thread && other)
		{
			debug_assert(handle == nullptr, "thread is still running");

			handle = std::exchange(thread.handle, nullptr);
			return *this;
		}

		bool Thread::valid() const
		{
			return handle != nullptr;
		}

		void Thread::join()
		{
			debug_assert(handle != nullptr, "thread must be initialized");

			WaitForSingleObject(handle, INFINITE);
			handle = nullptr;
		}
	}
}

#endif
