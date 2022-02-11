#pragma once

#include "config.h"

#if THREAD_USE_KERNEL32
# include <windows.h>
#elif THREAD_USE_PTHREAD
# include "core/sync/Mutex.hpp"
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
	class CriticalSection
	{
		friend class ConditionVariable;

	private:
		using this_type = CriticalSection;

	private:
#if THREAD_USE_PTHREAD
		/**  */
		Mutex mutex;
#elif THREAD_USE_KERNEL32
		/**  */
		CRITICAL_SECTION cs;
#endif

	public:
#if THREAD_USE_PTHREAD
		/**  */
		CriticalSection() = default;
#elif THREAD_USE_KERNEL32
		/**  */
		~CriticalSection();
		/**  */
		CriticalSection();
		/**
		 * \param[in] count The spin count.
		 *
		 * \note Currently only support with kernel32.
		 */
		explicit CriticalSection(unsigned int count);
#endif
		/**  */
		CriticalSection(const this_type &) = delete;
		/**  */
		this_type & operator = (const this_type &) = delete;

	public:
		/**  */
		void lock();
		/**  */
		void unlock();
#if THREAD_USE_KERNEL32
		/**
		 * \param[in] count The spin count.
		 * \return Previous spin count.
		 *
		 * \note Currently only support with kernel32.
		 */
		unsigned int setSpin(unsigned int count);
#endif
		/**  */
		void try_lock();
	};
}
}

namespace core
{
namespace sync
{
#if THREAD_USE_KERNEL32
	inline CriticalSection::~CriticalSection()
	{
		DeleteCriticalSection(&cs);
	}
	inline CriticalSection::CriticalSection()
	{
		InitializeCriticalSection(&cs);
	}
	inline CriticalSection::CriticalSection(const unsigned int count)
	{
		InitializeCriticalSectionAndSpinCount(&cs, count);
	}
#endif

	inline void CriticalSection::lock()
	{
#if THREAD_USE_KERNEL32
		EnterCriticalSection(&cs);
#elif THREAD_USE_PTHREAD
		mutex.lock();
#endif
	}
	inline void CriticalSection::unlock()
	{
#if THREAD_USE_KERNEL32
		LeaveCriticalSection(&cs);
#elif THREAD_USE_PTHREAD
		mutex.unlock();
#endif
	}
#if THREAD_USE_KERNEL32
	inline unsigned int CriticalSection::setSpin(const unsigned int count)
	{
		return SetCriticalSectionSpinCount(&cs, count);
	}
#endif
	inline void CriticalSection::try_lock()
	{
#if THREAD_USE_KERNEL32
		TryEnterCriticalSection(&cs);
#elif THREAD_USE_PTHREAD
		mutex.try_lock();
#endif
	}
}
}
