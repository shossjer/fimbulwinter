
#ifndef UTILITY_SPINLOCK_HPP
#define UTILITY_SPINLOCK_HPP

#include <atomic>

namespace utility
{
	class spinlock
	{
	private:
		std::atomic_flag flag = ATOMIC_FLAG_INIT;

	public:
		void lock()
		{
			while (this->flag.test_and_set());
		}
		void unlock()
		{
			this->flag.clear();
		}
	};
}

#endif /* UTILITY_SPINLOCK_HPP */
