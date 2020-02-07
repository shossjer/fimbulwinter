#include "config.h"

#if THREAD_USE_PTHREAD

#include "Thread.hpp"

#include "core/debug.hpp"

#include <algorithm>

#include <pthread.h>

namespace
{
	constexpr void * const invalid_data = nullptr;

	pthread_t get_pthread(void * data)
	{
		return static_cast<pthread_t>(reinterpret_cast<std::intptr_t>(data));
	}

	void * set_pthread(pthread_t pthread)
	{
		static_assert(sizeof(pthread_t) <= sizeof(void *), "pointer type is not large enough to hold pthread_t!");

		return reinterpret_cast<void *>(pthread);
	}

	void * start_thread(void * (* callback)(void *), void * arg)
	{
		pthread_t pthread;
		if (!debug_verify(pthread_create(&pthread, nullptr, callback, arg) == 0, "failed with errno ", errno))
			return invalid_data;

		return set_pthread(pthread);
	}

	void * call_void(void * arg)
	{
		((void(*)())arg)();

		return nullptr;
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
			: data_(start_thread(call_void, (void *)callback))
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

			debug_verify(pthread_join(get_pthread(data_), nullptr) == 0, "failed with errno ", errno);
			data_ = invalid_data;
		}
	}
}

#endif
