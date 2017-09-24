
#ifndef CORE_SYNC_MUTEX_HPP
#define CORE_SYNC_MUTEX_HPP

#include <config.h>

#if THREAD_USE_KERNEL32
# include <windows.h>
#elif THREAD_USE_PTHREAD
# include <pthread.h>
#endif

// so that we get std::lock_guard
#include <mutex>

namespace core
{
namespace sync
{
	class ConditionVariable;
}
}

namespace core
{
namespace sync
{
	class Mutex
	{
#if THREAD_USE_PTHREAD
		friend class ConditionVariable;
#endif

	private:
		using this_type = Mutex;

	private:
#if THREAD_USE_KERNEL32
		/**  */
		HANDLE hMutex;
#elif THREAD_USE_PTHREAD
		/**  */
		pthread_mutex_t mutex;
#endif

	public:
		/**  */
		~Mutex();
		/**  */
		Mutex();
		/**  */
		explicit Mutex(bool initial_owner);
		/**  */
		Mutex(const this_type &) = delete;
		/**  */
		this_type & operator = (const this_type &) = delete;

	public:
		/**  */
		void lock();
		/**  */
		void try_lock();
		/**  */
		void unlock();
	};
}
}


namespace core
{
namespace sync
{
	inline Mutex::~Mutex()
	{
#if THREAD_USE_KERNEL32
		CloseHandle(hMutex);
#elif THREAD_USE_PTHREAD
		pthread_mutex_destroy(&mutex);
#endif
	}
	inline Mutex::Mutex() : Mutex(false)
	{
	}
#if THREAD_USE_KERNEL32
	inline Mutex::Mutex(const bool initial_owner) :
		hMutex(CreateMutex(0, initial_owner, 0))
	{
	}
#elif THREAD_USE_PTHREAD
	inline Mutex::Mutex(const bool initial_owner)
	{
		pthread_mutex_init(&mutex, nullptr);
		if (initial_owner) lock();
	}
#endif

	inline void Mutex::lock()
	{
#if THREAD_USE_KERNEL32
		WaitForSingleObject(hMutex, INFINITE);
#elif THREAD_USE_PTHREAD
		pthread_mutex_lock(&mutex);
#endif
	}
	inline void Mutex::try_lock()
	{
#if THREAD_USE_KERNEL32
		WaitForSingleObject(hMutex, 0);
#elif THREAD_USE_PTHREAD
		pthread_mutex_trylock(&mutex);
#endif
	}
	inline void Mutex::unlock()
	{
#if THREAD_USE_KERNEL32
		ReleaseMutex(hMutex);
#elif THREAD_USE_PTHREAD
		pthread_mutex_unlock(&mutex);
#endif
	}
}
}

#endif /* CORE_SYNC_MUTEX_HPP */
