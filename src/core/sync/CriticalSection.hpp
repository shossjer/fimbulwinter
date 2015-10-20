
#ifndef CORE_SYNC_CRITICALSECTION_HPP
#define CORE_SYNC_CRITICALSECTION_HPP

#include <config.h>

#if THREAD_USE_KERNEL32
# include <windows.h>
#elif THREAD_USE_PTHREAD
# include "Mutex.hpp"
#endif

// so that we get std::lock_guard
#include <mutex>

namespace core
{
	namespace sync
	{
		class CriticalSection
		{
		private:
#if THREAD_USE_PTHREAD
			/**
			 */
			Mutex mutex;
#elif THREAD_USE_KERNEL32
			/**
			 */
			CRITICAL_SECTION cs;
#endif

		public:
			/**
			 */
			CriticalSection();
#if THREAD_USE_KERNEL32
			/**
			 * \param[in] count The spin count.
			 *
			 * \note Currently only support with kernel32.
			 */
			explicit CriticalSection(unsigned int count);
#endif
			/**
			 */
			CriticalSection(const CriticalSection &cs) = delete;
			/**
			 */
			~CriticalSection();

		public:
			/**
			 */
			void lock();
			/**
			 */
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
			/**
			 */
			void try_lock();
		};
	}
}

#if THREAD_USE_PTHREAD
inline core::sync::CriticalSection::CriticalSection() :
	mutex()
{
}
#elif THREAD_USE_KERNEL32
inline core::sync::CriticalSection::CriticalSection()
{
	InitializeCriticalSection(&this->cs);
}
inline core::sync::CriticalSection::CriticalSection(const unsigned int count)
{
	InitializeCriticalSectionAndSpinCount(&this->cs, count);
}
#endif
inline core::sync::CriticalSection::~CriticalSection()
{
#if THREAD_USE_KERNEL32
	DeleteCriticalSection(&this->cs);
#endif
}

inline void core::sync::CriticalSection::lock()
{
#if THREAD_USE_PTHREAD
	this->mutex.lock();
#elif THREAD_USE_KERNEL32
	EnterCriticalSection(&this->cs);
#endif
}
inline void core::sync::CriticalSection::unlock()
{
#if THREAD_USE_PTHREAD
	this->mutex.unlock();
#elif THREAD_USE_KERNEL32
	LeaveCriticalSection(&this->cs);
#endif
}
#if THREAD_USE_KERNEL32
inline unsigned int core::sync::CriticalSection::setSpin(const unsigned int count)
{
	return SetCriticalSectionSpinCount(&this->cs, count);
}
#endif
inline void core::sync::CriticalSection::try_lock()
{
#if THREAD_USE_PTHREAD
	this->mutex.try_lock();
#elif THREAD_USE_KERNEL32
	TryEnterCriticalSection(&this->cs);
#endif
}

#endif /* CORE_SYNC_CRITICALSECTION_HPP */
