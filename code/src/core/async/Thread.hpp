
#ifndef CORE_ASYNC_THREAD_HPP
#define CORE_ASYNC_THREAD_HPP

#include <config.h>

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
		 */
		class Thread
		{
		private:
#if THREAD_USE_KERNEL32
			/**
			 */
			HANDLE handle;
#elif THREAD_USE_PTHREAD
			/**
			 */
			pthread_t pthread;
#endif

		public:
			/**
			 */
			Thread();
			/**
			 */
			Thread(void (*const callback)());
			/**
			 */
			Thread(const Thread &thread) = delete;
			/**
			 */
			Thread(Thread &&thread);
			/**
			 */
			~Thread();

		public:
			/**
			 */
			Thread &operator = (const Thread &thread) = delete;
			/**
			 */
			Thread &operator = (Thread &&thread);

		public:
			/**
			 */
			void join();
		private:
#if THREAD_USE_KERNEL32
		/**
		 */
		void create(DWORD WINAPI callback(LPVOID), LPVOID lpVoid);
#elif THREAD_USE_PTHREAD
		/**
		 */
		void create(void *(*const callback)(void *), void *const arg);
#endif
		};
	}
}

#endif /* ENGINE_ASYNC_THREAD_HPP */
