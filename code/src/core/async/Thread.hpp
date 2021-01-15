#pragma once

#if THREAD_USE_KERNEL32
# include <Windows.h>
#endif

namespace core
{
	namespace async
	{
#if THREAD_USE_KERNEL32
# define thread_decl WINAPI

		using thread_return = DWORD;
		using thread_param = LPVOID;
#elif THREAD_USE_PTHREAD
# define thread_decl

		using thread_return = void *;
		using thread_param = void *;
#endif


		class Thread
		{
		private:

			void * data_;

		public:

			~Thread();
			explicit Thread();
			explicit Thread(void (* callback)());
#ifdef thread_decl
			// note not platform independent
			explicit Thread(thread_return (* callback)(thread_param), thread_param arg);
#endif
			Thread(Thread && other);
			Thread & operator = (Thread && other);

		public:

			bool valid() const;

			void join();
		};
	}
}
