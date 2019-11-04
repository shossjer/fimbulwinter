
#ifndef CORE_CONTAINER_CIRCLEQUEUE_HPP
#define CORE_CONTAINER_CIRCLEQUEUE_HPP

#include <utility/array_alloc.hpp>
#include <utility/spinlock.hpp>
#include <utility/type_traits.hpp>

#include <atomic>
#include <mutex>
#include <thread>

namespace core
{
	namespace container
	{
		struct single_read_single_write;
		struct single_read_multiple_write;

		/**
		 * \tparam T      Type.
		 * \tparam N      Count.
		 * \tparam Policy Synchronization mechanism between threads.
		 */
		template <typename T, std::size_t N, typename Policy = single_read_single_write>
		class CircleQueue;

		template <typename T, std::size_t N>
		class CircleQueue<T, N, single_read_single_write>
		{
		public:
			static constexpr int capacity = N;

		public:
			std::atomic_int begini;
			std::atomic_int endi;
			utility::static_storage<N, T> buffer;

		public:
			CircleQueue() :
				begini{0},
				endi{0}
			{}

		public:
			/**
			 * \note May only be called one thread at a time.
			 *
			 * \param[in] ps Construction parameters.
			 * \return True on success, false otherwise.
			 */
			template <typename ...Ps>
			bool try_emplace(Ps && ...ps)
			{
				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_relaxed);

				int next_endi = endi + 1;
				if (next_endi == this->capacity) next_endi = 0;
				if (begini == next_endi)
					return false;

				buffer.construct_at(endi, std::forward<Ps>(ps)...);
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
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

				item = std::move(buffer[begini]);
				buffer.destruct_at(begini);

				int next_begini = begini + 1;
				if (next_begini == this->capacity)
					next_begini = 0;
				this->begini.store(next_begini, std::memory_order_relaxed);
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
				return try_emplace(item);
			}
			/**
			 * \note May only be called one thread at a time.
			 *
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(T && item)
			{
				return try_emplace(std::move(item));
			}
		};
		
		template <typename T, std::size_t N>
		class CircleQueue<T, N, single_read_multiple_write>
		{
		public:
			static constexpr int capacity = N;

		public:
			std::atomic_int begini;
			std::atomic_int endi;
			utility::static_storage<N, T> buffer;
			utility::spinlock writelock;

		public:
			CircleQueue() :
				begini{0},
				endi{0}
			{}

		public:
			/**
			 * \param[in] ps Construction parameters.
			 * \return True on success, false otherwise.
			 */
			template <typename ...Ps>
			bool try_emplace(Ps && ...ps)
			{
				// I had planed to  do something more complicated than
				// this, but  it took a  lot of time and  was possibly
				// wrong  also  so  I   did  this  very  simple  thing
				// instead. -- shossjer
				std::lock_guard<utility::spinlock> lock{this->writelock};

				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_relaxed);

				int next_endi = endi + 1;
				if (next_endi == this->capacity) next_endi = 0;
				if (begini == next_endi)
					return false;

				buffer.construct_at(endi, std::forward<Ps>(ps)...);
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
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

				item = std::move(buffer[begini]);
				buffer.destruct_at(begini);

				int next_begini = begini + 1;
				if (next_begini == this->capacity)
					next_begini = 0;
				this->begini.store(next_begini, std::memory_order_relaxed);
				return true;
			}
			/**
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(const T & item)
			{
				return try_emplace(item);
			}
			/**
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(T && item)
			{
				return try_emplace(std::move(item));
			}
		};

		template <typename T, std::size_t N>
		using CircleQueueSRSW = CircleQueue<T, N, single_read_single_write>;
		template <typename T, std::size_t N>
		using CircleQueueSRMW = CircleQueue<T, N, single_read_multiple_write>;
	}
}

#endif /* CORE_CONTAINER_CIRCLEQUEUE_HPP */
