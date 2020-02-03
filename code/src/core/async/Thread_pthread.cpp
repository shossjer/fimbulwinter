#include "config.h"

#if THREAD_USE_PTHREAD

#include "Thread.hpp"

#include "core/debug.hpp"

#include <algorithm>

namespace
{
	void * call_void(void * arg)
	{
		((void(*)())arg)();

		return nullptr;
	}

	constexpr auto invalid_pthread = pthread_t{};

	pthread_t create_thread(void * callback(void *), void * arg)
	{
		pthread_t pthread;
		if (!debug_verify(pthread_create(&pthread, nullptr, callback, arg) == 0, "pthread_create failed"))
			return invalid_pthread;

		return pthread;
	}
}

namespace core
{
	namespace async
	{
		Thread::~Thread()
		{
			debug_assert(pthread == invalid_pthread, "thread might still be running");
		}

		Thread::Thread()
			: pthread(invalid_pthread)
		{}

		Thread::Thread(void (* callback)())
			: pthread(create_thread(call_void, (void *)callback))
		{}

		Thread::Thread(Thread && other)
			: pthread(std::exchange(other.pthread, invalid_pthread))
		{}

		Thread & Thread::operator = (Thread && other)
		{
			debug_assert(pthread == invalid_pthread);

			pthread = std::exchange(other.pthread, invalid_pthread);
			return *this;
		}

		bool Thread::valid() const
		{
			return pthread != invalid_pthread;
		}

		void Thread::join()
		{
			debug_assert(pthread != invalid_pthread, "thread must be initialized");

			pthread_join(pthread, nullptr);
			pthread = invalid_pthread;
		}
	}
}

#endif
