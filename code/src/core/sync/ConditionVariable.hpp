
#ifndef CORE_SYNC_CONDITIONVARIABLE_HPP
#define CORE_SYNC_CONDITIONVARIABLE_HPP

#include <config.h>

# include "CriticalSection.hpp"

#if THREAD_USE_KERNEL32
# include <Windows.h>
#elif THREAD_USE_PTHREAD
# include "Mutex.hpp"
# include <pthread.h>
#endif

namespace core
{
namespace sync
{
	/**  */
	class ConditionVariable
	{
	private:
		using this_type = ConditionVariable;

	private:
#if THREAD_USE_KERNEL32
		/**  */
		CONDITION_VARIABLE cv;
#elif THREAD_USE_PTHREAD
		/**  */
		pthread_cond_t cond;
#endif

	public:
#if THREAD_USE_PTHREAD
		/**  */
		~ConditionVariable();
#endif
		/**  */
		ConditionVariable();
		/**  */
		ConditionVariable(const this_type &) = delete;
		/**  */
		this_type & operator = (const this_type &) = delete;

	public:
		/**  */
		bool notify_all();
		/**  */
		bool notify_one();
		/**  */
		bool wait(CriticalSection & cs);
#if THREAD_USE_PTHREAD
		/**  */
		bool wait(Mutex & mutex);
#endif
	};
}
}

namespace core
{
namespace sync
{
#if THREAD_USE_PTHREAD
	inline ConditionVariable::~ConditionVariable()
	{
		pthread_cond_destroy(&cond);
	}
#endif
	inline ConditionVariable::ConditionVariable()
	{
#if THREAD_USE_KERNEL32
		InitializeConditionVariable(&cv);
#elif THREAD_USE_PTHREAD
		pthread_cond_init(&cond, nullptr);
#endif
	}

	inline bool ConditionVariable::notify_all()
	{
#if THREAD_USE_KERNEL32
		WakeAllConditionVariable(&cv);
		return true;
#elif THREAD_USE_PTHREAD
		return pthread_cond_broadcast(&cond) == 0;
#endif
	}
	inline bool ConditionVariable::notify_one()
	{
#if THREAD_USE_KERNEL32
		WakeConditionVariable(&cv);
		return true;
#elif THREAD_USE_PTHREAD
		return pthread_cond_signal(&cond) == 0;
#endif
	}
	inline bool ConditionVariable::wait(CriticalSection & cs)
	{
#if THREAD_USE_KERNEL32
		return SleepConditionVariableCS(&cv, &cs.cs, INFINITE) == TRUE;
#elif THREAD_USE_PTHREAD
		return wait(cs.mutex);
#endif
	}
#if THREAD_USE_PTHREAD
	inline bool ConditionVariable::wait(Mutex & mutex)
	{
		return pthread_cond_wait(&cond, &mutex.mutex) == 0;
	}
#endif
}
}

#endif /* CORE_SYNC_CONDITIONVARIABLE_HPP */
