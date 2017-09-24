
#ifndef CORE_SYNC_EVENT_HPP
#define CORE_SYNC_EVENT_HPP

#include <config.h>

#if THREAD_USE_KERNEL32
# include <Windows.h>
#else
# include "ConditionVariable.hpp"
# include "Mutex.hpp"
#endif

namespace core
{
namespace sync
{
	/** ManualReset - to use manual reset or not */
	template <bool ManualReset = false>
	class Event
	{
	private:
		using this_type = Event<ManualReset>;

	private:
#if THREAD_USE_KERNEL32
		/**  */
		HANDLE hEvent;
#else
		/**  */
		Mutex mutex;
		/**  */
		ConditionVariable cond;
		/**  */
		volatile bool is_set;
#endif

	public:
#if THREAD_USE_KERNEL32
		/**  */
		~Event();
#endif
		/**  */
		Event();
		/**  */
		Event(const this_type &) = delete;
		/**  */
		this_type & operator = (const this_type &) = delete;
		/**  */
		explicit Event(bool initial_state);

	public:
		/**  */
		bool set();

		/**  */
		bool wait();
	};
	/** true - to use manual reset or not */
	template <>
	class Event <true>
	{
	private:
		using this_type = Event<true>;

	private:
#if THREAD_USE_KERNEL32
		/**  */
		HANDLE hEvent;
#else
		/**  */
		Mutex mutex;
		/**  */
		ConditionVariable cond;
		/**  */
		volatile bool is_set;
#endif

	public:
#if THREAD_USE_KERNEL32
		/**  */
		~Event();
#endif
		/**  */
		Event();
		/**  */
		Event(const this_type &) = delete;
		/**  */
		this_type & operator = (const this_type &) = delete;
		/**  */
		explicit Event(bool initial_state);

	public:
		/**  */
		bool reset();
		/**  */
		bool set();

		/**  */
		bool wait();
	};
}
}

namespace core
{
namespace sync
{
#if THREAD_USE_KERNEL32
	template <bool ManualReset>
	inline Event<ManualReset>::~Event()
	{
		CloseHandle(hEvent);
	}
#endif
	template <bool ManualReset>
	inline Event<ManualReset>::Event() : Event(false)
	{
	}
#if THREAD_USE_KERNEL32
	template <bool ManualReset>
	inline Event<ManualReset>::Event(const bool initial_state) :
		hEvent(CreateEvent(nullptr, FALSE, initial_state, nullptr))
	{
	}
#else
	template <bool ManualReset>
	inline Event<ManualReset>::Event(const bool initial_state) :
		is_set(initial_state)
	{
	}
#endif

	template <bool ManualReset>
	inline bool Event<ManualReset>::set()
	{
#if THREAD_USE_KERNEL32
		return SetEvent(hEvent) == TRUE;
#else
		std::lock_guard<Mutex> lock{mutex};
		is_set = true;
		cond.notify_one();
		return true;
#endif
	}

	template <bool ManualReset>
	inline bool Event<ManualReset>::wait()
	{
#if THREAD_USE_KERNEL32
		return WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
#else
		std::lock_guard<Mutex> lock{mutex};
		while (!is_set) cond.wait(mutex);
		is_set = false;
		return true;
#endif
	}

#if THREAD_USE_KERNEL32
	inline Event<true>::~Event()
	{
		CloseHandle(hEvent);
	}
#endif
	inline Event<true>::Event() : Event(false)
	{
	}
#if THREAD_USE_KERNEL32
	inline Event<true>::Event(const bool initial_state) :
		hEvent(CreateEvent(nullptr, TRUE, initial_state, nullptr))
	{
	}
#else
	inline Event<true>::Event(const bool initial_state) :
		is_set(initial_state)
	{
	}
#endif

	inline bool Event<true>::reset()
	{
#if THREAD_USE_KERNEL32
		return ResetEvent(hEvent) == TRUE;
#else
		std::lock_guard<Mutex> lock{mutex};
		is_set = false;
		return true;
#endif
	}
	inline bool Event<true>::set()
	{
#if THREAD_USE_KERNEL32
		return SetEvent(hEvent) == TRUE;
#else
		std::lock_guard<Mutex> lock{mutex};
		is_set = true;
		cond.notify_one();
		return true;
#endif
	}

	inline bool Event<true>::wait()
	{
#if THREAD_USE_KERNEL32
		return WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
#else
		std::lock_guard<Mutex> lock{mutex};
		while (!is_set) cond.wait(mutex);
		return true;
#endif
	}
}
}

#endif /* CORE_SYNC_EVENT_HPP */