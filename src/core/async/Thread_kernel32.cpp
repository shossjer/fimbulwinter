
#include <core/debug.hpp>

#include <windows.h>

#include <stdexcept>

namespace
{
	DWORD WINAPI call_void(LPVOID arg)
	{
		((void(*)())arg)();

		return DWORD{0};
	}
}

namespace core
{
	namespace async
	{
		Thread::Thread() :
			handle(nullptr)
		{
		}
		Thread::Thread(void (*const callback)())
		{
			this->create(call_void, (LPVOID)callback);
		}
		Thread::Thread(Thread &&thread) :
			handle(thread.handle)
		{
			thread.handle = nullptr;
		}
		Thread::~Thread()
		{
			debug_assert(this->handle == nullptr);
		}

		Thread &Thread::operator = (Thread &&thread)
		{
			debug_assert(this->handle == nullptr);

			this->handle = thread.handle;
			thread.handle = nullptr;
			return *this;
		}

		void Thread::join()
		{
			WaitForSingleObject(this->handle, INFINITE);
			this->handle = nullptr;
		}

		void Thread::create(DWORD WINAPI callback(LPVOID), LPVOID lpVoid)
		{
			this->handle = CreateThread(nullptr, 0, callback, lpVoid, 0, nullptr);

			if (handle == nullptr)
				throw std::runtime_error("CreateThread failed");
		}
	}
}
