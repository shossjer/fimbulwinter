#include "config.h"

#if THREAD_USE_KERNEL32

#include "Thread.hpp"

#include "core/debug.hpp"

#include <algorithm>

#include <windows.h>

namespace
{
	constexpr void * const invalid_data = nullptr;

	HANDLE get_handle(void * data)
	{
		return static_cast<HANDLE>(data);
	}

	void * set_handle(HANDLE handle)
	{
		return static_cast<void *>(handle);
	}

	void * start_thread(LPTHREAD_START_ROUTINE callback, LPVOID lpVoid)
	{
		HANDLE handle = CreateThread(nullptr, 0, callback, lpVoid, 0, nullptr);
		debug_verify(handle != nullptr, "CreateThread failed");
		return set_handle(handle);
	}

	DWORD WINAPI call_void(LPVOID arg)
	{
		((void(*)())arg)();

		return DWORD{};
	}
}

namespace core
{
	namespace async
	{
		Thread::~Thread()
		{
			debug_assert(data_ == invalid_data, "thread resource lost");
		}

		Thread::Thread()
			: data_(invalid_data)
		{}

		Thread::Thread(void (* callback)())
			: data_(start_thread(call_void, (LPVOID)callback))
		{}

		Thread::Thread(Thread && other)
			: data_(std::exchange(other.data_, invalid_data))
		{}

		Thread & Thread::operator = (Thread && other)
		{
			debug_assert(data_ == invalid_data, "thread resource lost");

			data_ = std::exchange(other.data_, invalid_data);
			return *this;
		}

		bool Thread::valid() const
		{
			return data_ != invalid_data;
		}

		void Thread::join()
		{
			debug_assert(data_ != invalid_data, "thread is not joinable");

			WaitForSingleObject(get_handle(data_), INFINITE); // todo check error
			data_ = invalid_data;
		}
	}
}

#endif
