
#ifndef CORE_CONTAINER_CIRCLEQUEUE_HPP
#define CORE_CONTAINER_CIRCLEQUEUE_HPP

#include <utility/spinlock.hpp>

#include <atomic>
#include <thread>
#include <type_traits>

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
			//T buffer[N];
			std::aligned_storage_t<sizeof(T), alignof(T)> buffer[N];

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

				//item = this->buffer[begini];
				item = *reinterpret_cast<T *>(this->buffer + begini);
				reinterpret_cast<T *>(this->buffer + begini)->~T();

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
				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_relaxed);

				int next_endi = endi + 1;
				if (next_endi == this->capacity) next_endi = 0;
				if (begini == next_endi)
					return false;

				new (this->buffer + endi) T(item);
				//this->buffer[endi] = new (this->buffer + endi) T(item);
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
			/**
			 * \note May only be called one thread at a time.
			 *
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(T && item)
			{
				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_relaxed);

				int next_endi = endi + 1;
				if (next_endi == this->capacity) next_endi = 0;
				if (begini == next_endi)
					return false;

				new (this->buffer + endi) T(std::move(item));
				//this->buffer[endi] = new (this->buffer + endi) T(item);
				this->endi.store(next_endi, std::memory_order_release);
				return true;
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
			//T buffer[N];
			std::aligned_storage_t<sizeof(T), alignof(T)> buffer[N];
			utility::spinlock writelock;

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

				//item = this->buffer[begini];
				item = *reinterpret_cast<T *>(this->buffer + begini);
				reinterpret_cast<T *>(this->buffer + begini)->~T();

				int next_begini = begini + 1;
				if (next_begini == this->capacity)
					next_begini = 0;
				this->begini.store(next_begini, std::memory_order_relaxed);
				return true;
			}
			bool try_pop2(T & item)
			{
				const int begini = this->begini.load(std::memory_order_relaxed);
				const int endi = this->endi.load(std::memory_order_acquire);
				if (begini == endi)
					return false;

				//item = this->buffer[begini];
				item = std::move(*reinterpret_cast<T *>(this->buffer + begini));

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

				new (this->buffer + endi) T(item);
				//this->buffer[endi] = item;
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
			/**
			 * \param[in] item The thing to push.
			 * \return True on success, false otherwise.
			 */
			bool try_push(T && item)
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

				new (this->buffer + endi) T(std::move(item));
				//this->buffer[endi] = item;
				this->endi.store(next_endi, std::memory_order_release);
				return true;
			}
		};

		template <typename T, std::size_t N>
		using CircleQueueSRSW = CircleQueue<T, N, single_read_single_write>;
		template <typename T, std::size_t N>
		using CircleQueueSRMW = CircleQueue<T, N, single_read_multiple_write>;
	}
}

#endif /* CORE_CONTAINER_CIRCLEQUEUE_HPP */
