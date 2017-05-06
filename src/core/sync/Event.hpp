
#ifndef CORE_SYNC_EVENT_HPP
#define CORE_SYNC_EVENT_HPP

#include "config.h"

#if THREAD_USE_PTHREAD
# include "Mutex.hpp"
# include "ConditionalVariable.hpp"
#elif THREAD_USE_KERNEL32
# include <Windows.h>
#endif

namespace core
{
namespace sync
{
 	/** ManualReset - to use manual reset or not
 		Dummy - dummy */
 	template <bool ManualReset = false, class Dummy = int>
 	class Event
 	{
 	public:
 	protected:
 	private:
#if THREAD_USE_PTHREAD
 		/**  */
 		::thread::sync::Mutex mutex;
 		/**  */
 		::thread::sync::ConditionalVariable cond;
 		/**  */
 		bool isSet;
#elif THREAD_USE_KERNEL32
 		/**  */
 		HANDLE hEvent;
#endif
			
 	public:
 		/**  */
 		Event();
 		/**  */
 		explicit Event(bool initial_state);
 		/**  */
 		~Event();
 	private:
 		/**  */
 		Event(const Event<ManualReset> &event_);
			
 	public:
 		/**  */
 		bool set();

 		/**  */
 		bool wait();
 	protected:
 	private:
			
 	};
 	/** true - to use manual reset or not
 		Dummy - dummy */
 	template <class Dummy>
 	class Event <true, Dummy>
 	{
 	public:
 	protected:
 	private:
#if THREAD_USE_PTHREAD
 		/**  */
 		::thread::sync::Mutex mutex;
 		/**  */
 		::thread::sync::ConditionalVariable cond;
 		/**  */
 		bool isSet;
#elif THREAD_USE_KERNEL32
 		/**  */
 		HANDLE hEvent;
#endif
			
 	public:
 		/**  */
 		Event();
 		/**  */
 		explicit Event(bool initial_state);
 		/**  */
 		~Event();
 	private:
 		/**  */
 		Event(const Event<true> &event_);
			
 	public:
 		/**  */
 		bool reset();
 		/**  */
 		bool set();

 		/**  */
 		bool wait();
 	protected:
 	private:
			
 	};
}
}

namespace core
{
	namespace sync
	{
		/**  */
#if THREAD_USE_PTHREAD
		template <bool ManualReset, class Dummy>
		inline Event<ManualReset, Dummy>::Event() :
			mutex(),
			cond(),
			isSet(false)
		{
		}
		template <bool ManualReset, class Dummy>
		inline Event<ManualReset, Dummy>::Event(const bool initial_state) :
			mutex(),
			cond(),
			isSet(initial_state)
		{
		}
#elif THREAD_USE_KERNEL32
		template <bool ManualReset, class Dummy>
		inline Event<ManualReset, Dummy>::Event() :
			hEvent(CreateEvent(NULL, FALSE, FALSE, NULL))
		{
		}
		template <bool ManualReset, class Dummy>
		inline Event<ManualReset, Dummy>::Event(const bool initial_state) :
			hEvent(CreateEvent(NULL, FALSE, initial_state, NULL))
		{
		}
#endif
		template <bool ManualReset, class Dummy>
		inline Event<ManualReset, Dummy>::~Event()
		{
#if THREAD_USE_KERNEL32
			CloseHandle(hEvent);
#endif
		}
		
		template <bool ManualReset, class Dummy>
		inline bool Event<ManualReset, Dummy>::set()
		{
#if THREAD_USE_PTHREAD
			mutex.lock();
			
			isSet = true;
			cond.signal();
			
			mutex.unlock();
			
			return true;
#elif THREAD_USE_KERNEL32
			return SetEvent(hEvent) == TRUE;
#endif
		}

		template <bool ManualReset, class Dummy>
		inline bool Event<ManualReset, Dummy>::wait()
		{
#if THREAD_USE_PTHREAD
			mutex.lock();
			
			while (!isSet) cond.wait(mutex);
			
			isSet = false;
			
			mutex.unlock();
			
			return true;
#elif THREAD_USE_KERNEL32
			return WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
#endif
		}
		/**  */
#if THREAD_USE_PTHREAD
		template <class Dummy>
		inline Event<true, Dummy>::Event() :
			mutex(),
			cond(),
			isSet(false)
		{
		}
		template <class Dummy>
		inline Event<true, Dummy>::Event(const bool initial_state) :
			mutex(),
			cond(),
			isSet(initial_state)
		{
		}
#elif THREAD_USE_KERNEL32
		template <class Dummy>
		inline Event<true, Dummy>::Event() :
			hEvent(CreateEvent(NULL, TRUE, FALSE, NULL))
		{
		}
		template <class Dummy>
		inline Event<true, Dummy>::Event(const bool initial_state) :
			hEvent(CreateEvent(NULL, TRUE, initial_state, NULL))
		{
		}
#endif
		template <class Dummy>
		inline Event<true, Dummy>::~Event()
		{
#if THREAD_USE_KERNEL32
			CloseHandle(hEvent);
#endif
		}
		
		template <class Dummy>
		inline bool Event<true, Dummy>::reset()
		{
#if THREAD_USE_PTHREAD
			mutex.lock();
			
			isSet = false;
			
			mutex.unlock();
			
			return true;
#elif THREAD_USE_KERNEL32
			return ResetEvent(hEvent) == TRUE;
#endif
		}
		template <class Dummy>
		inline bool Event<true, Dummy>::set()
		{
#if THREAD_USE_PTHREAD
			mutex.lock();
			
			isSet = true;
			cond.signal(),
				
				mutex.unlock();
			
			return true;
#elif THREAD_USE_KERNEL32
			return SetEvent(hEvent) == TRUE;
#endif
		}

		template <class Dummy>
		inline bool Event<true, Dummy>::wait()
		{
#if THREAD_USE_PTHREAD
			mutex.lock();
			
			while (!isSet) cond.wait(mutex);
			
			mutex.unlock();
			
			return true;
#elif THREAD_USE_KERNEL32
			return WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
#endif
		}
	}
}


#endif /* CORE_SYNC_EVENT_HPP */
