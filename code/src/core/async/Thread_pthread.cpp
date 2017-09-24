
#include <core/debug.hpp>

#include <pthread.h>

#include <stdexcept>

namespace
{
	void *call_void(void *arg)
	{
		((void(*)())arg)();

		return nullptr;
	}
}

namespace core
{
	namespace async
	{
		Thread::Thread() :
			pthread(0)
		{
		}
		Thread::Thread(void (*const callback)())
		{
			this->create(call_void, (void *)callback);
		}
		Thread::Thread(Thread &&thread) :
			pthread(thread.pthread)
		{
			thread.pthread = 0;
		}
		Thread::~Thread()
		{
			debug_assert(this->pthread == std::size_t{0});
		}

		Thread &Thread::operator = (Thread &&thread)
		{
			debug_assert(this->pthread == std::size_t{0});

			this->pthread = thread.pthread;
			thread.pthread = 0;
			return *this;
		}

		void Thread::join()
		{
			pthread_join(this->pthread, nullptr);
			this->pthread = 0;
		}

		void Thread::create(void *callback(void *), void *arg)
		{
			const auto ret = pthread_create(&this->pthread, nullptr, callback, arg);

			if (ret != 0)
				throw std::runtime_error("pthread_create failed");
		}
	}
}
