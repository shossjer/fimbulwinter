
#ifndef CORE_THREAD_SYNC_MUTEX_HPP
#define CORE_THREAD_SYNC_MUTEX_HPP

#include <config.h>

#if THREAD_USE_KERNEL32
# include <Windows.h>
#elif THREAD_USE_PTHREAD
# include <pthread.h>
#endif

// so that we get std::lock_guard
#include <mutex>

namespace core
{
	namespace thread
	{
		namespace sync
		{
			class Mutex
			{
			private:
#if THREAD_USE_KERNEL32
				/**
				 */
				HANDLE hMutex;
#elif THREAD_USE_PTHREAD
				/**
				 */
				pthread_mutex_t mutex;
#endif

			public:
				/**
				 */
				Mutex();
				/**
				 */
				explicit Mutex(bool initial_owner);
				/**
				 */
				Mutex(const Mutex &mutex) = delete;
				/**
				 */
				~Mutex();

			public:
				/**
				 */
				void lock();
				/**
				 */
				void try_lock();
				/**
				 */
				void unlock();
			};
		}
	}
}

#if THREAD_USE_PTHREAD
inline core::thread::sync::Mutex::Mutex()
{
	pthread_mutex_init(&this->mutex, nullptr);
}
inline core::thread::sync::Mutex::Mutex(const bool initial_owner)
{
	pthread_mutex_init(&this->mutex, nullptr);
	
	if (initial_owner) lock();
}
#elif THREAD_USE_KERNEL32
inline core::thread::sync::Mutex::Mutex() :
	hMutex(CreateMutex(0, FALSE, 0))
{
}
inline core::thread::sync::Mutex::Mutex(const bool initial_owner) :
	hMutex(CreateMutex(0, initial_owner, 0))
{
}
#endif
inline core::thread::sync::Mutex::~Mutex()
{
#if THREAD_USE_PTHREAD
	pthread_mutex_destroy(&this->mutex);
#elif THREAD_USE_WINDOWS
	CloseHandle(this->hMutex);
#endif
}
		
inline void core::thread::sync::Mutex::lock()
{
#if THREAD_USE_PTHREAD
	pthread_mutex_lock(&this->mutex);
#elif THREAD_USE_KERNEL32
	WaitForSingleObject(this->hMutex, INFINITE);
#endif
}
inline void core::thread::sync::Mutex::try_lock()
{
#if THREAD_USE_PTHREAD
	pthread_mutex_trylock(&this->mutex);
#elif THREAD_USE_KERNEL32
	WaitForSingleObject(this->hMutex, 0);
#endif
}
inline void core::thread::sync::Mutex::unlock()
{
#if THREAD_USE_PTHREAD
	pthread_mutex_unlock(&this->mutex);
#elif THREAD_USE_KERNEL32
	ReleaseMutex(this->hMutex);
#endif
}

#endif /* CORE_THREAD_SYNC_MUTEX_HPP */
