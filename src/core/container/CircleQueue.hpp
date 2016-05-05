
#ifndef CORE_CONTAINER_CIRCLEQUEUE_HPP
#define CORE_CONTAINER_CIRCLEQUEUE_HPP

#include <atomic>

namespace core
{
	namespace container
	{
		template <typename T, std::size_t N>
		class CircleQueue
		{
		public:
			static constexpr int capacity = N;

		public:
			std::atomic_int begini;
			std::atomic_int endi;
			T buffer[N];

		public:
			CircleQueue() :
				begini{0},
				endi{0}
			{
			}

		public:
			/**
			 * \note May only be called one thread at a time.
			 *
			 * \param[out] item Where to write on success.
			 * \return True on success, false otherwise.
			 */
			bool try_pop(T & item)
			{
				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_acquire);
				if (begini == endi)
					return false;

				item = this->buffer[begini];

				int next_begini = begini + 1;
				if (next_begini == this->capacity)
					next_begini = 0;
				this->begini.store(next_begini, std::memory_order_release);
				return true;
			}
			/**
			 * \note May only be called one thread at a time.
			 *
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(const T & item)
			{
				const int begini = this->begini.load(std::memory_order_acquire);
				const int endi = this->endi.load(std::memory_order_relaxed);

				int next_endi = endi + 1;
				if (next_endi == this->capacity) next_endi = 0;
				if (begini == next_endi)
					return false;

				this->buffer[endi] = item;
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
		};
	}
}

#endif /* CORE_CONTAINER_CIRCLEQUEUE_HPP */
