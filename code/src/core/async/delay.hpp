#pragma once

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
		/**
		 * Delays calling thread.
		 *
		 * \warning Not interrupt-safe on Linux.
		 *
		 * \param[in] millisec Number of milliseconds to delay.
		 *
		 * \note There are often better ways to debug your code than to insert a
		 *   bunch of delays everywhere; such as running the debugger, keeping a
		 *   log or becomming a better programmer. Use this function only where
		 *   absolutely necessary.
		 */
		inline void delay(unsigned int millisec)
		{
#if THREAD_USE_KERNEL32
			Sleep((DWORD)millisec);
#elif THREAD_USE_PTHREAD
			const unsigned int sec = millisec / 1000;
			millisec = millisec % 1000;

			const struct timespec sleep_time = {sec, millisec * 1000000};

			clock_nanosleep(CLOCK_REALTIME, 0, &sleep_time, NULL);
#endif
		}

		inline void yield()
		{
#if THREAD_USE_KERNEL32
			SwitchToThread();
#elif THREAD_USE_PTHREAD
			pthread_yield();
#endif
		}
	}
}
