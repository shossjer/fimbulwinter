
#ifndef CORE_ASYNC_THREAD_HPP
#define CORE_ASYNC_THREAD_HPP

#include "config.h"

#if THREAD_USE_KERNEL32
# include <windows.h>
#elif THREAD_USE_PTHREAD
# include <pthread.h>
#endif

namespace core
{
	namespace async
	{
		class Thread
		{
		private:
#if THREAD_USE_KERNEL32
			HANDLE handle;
#elif THREAD_USE_PTHREAD
			pthread_t pthread;
#endif

		public:
			~Thread();
			Thread();
			Thread(void (* callback)());
			Thread(Thread && other);
			Thread & operator = (Thread && other);

		public:
			bool valid() const;

			void join();
		};
	}
}

#endif /* ENGINE_ASYNC_THREAD_HPP */
