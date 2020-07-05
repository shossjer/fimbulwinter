
#ifndef CORE_CONTAINER_EXCHANGEQUEUE_HPP
#define CORE_CONTAINER_EXCHANGEQUEUE_HPP

#include "utility/spinlock.hpp"
#include "utility/storage.hpp"

#include <atomic>
#include <bitset>
#include <thread>
#include <type_traits>

namespace core
{
	namespace container
	{
		struct single_read_single_write;
		struct single_read_multiple_write;

		/**
		 * Contrary to  the CircleQueue, only the  latest written data
		 * can be polled from the ExchangeQueue.
		 *
		 * \tparam T      Type.
		 * \tparam Policy Synchronization mechanism between threads.
		 */
		template <typename T, typename Policy = single_read_single_write>
		class ExchangeQueue;

		template <typename T>
		class ExchangeQueue<T, single_read_single_write>
		{
		private:
			std::atomic_int lasti;
			int readi;
			int writei;
			std::bitset<3> writebits;
			utility::static_storage<3, T> buffer_;

		public:
			ExchangeQueue() :
				lasti(0),
				readi(1),
				writei(2)
			{}

		public:
			/**
			 * \param[out] item Where to write on success.
			 * \return True on success, false otherwise.
			 */
			bool try_pop(T & item)
			{
				readi = lasti.exchange(readi, std::memory_order_acquire);
				if (writebits.test(readi))
				{
					using utility::iter_move;
					item = iter_move(buffer_.data(buffer_.begin()) + readi);
					writebits.reset(readi);
					buffer_.destruct_at(buffer_.begin() + readi);
					return true;
				}
				return false;
			}
			/**
			 * \param[in] ps The thing to push.
			 * \return Always true.
			 */
			template <typename ...Ps>
			bool try_push(Ps && ...ps)
			{
				writebits.set(writei);
				buffer_.construct_at_(buffer_.begin() + writei, std::forward<Ps>(ps)...);
				writei = lasti.exchange(writei, std::memory_order_release);
				// remove any unread item
				if (writebits.test(writei))
				{
					buffer_.destruct_at(buffer_.begin() + writei);
				}
				return true;
			}
		};

		template <typename T>
		class ExchangeQueue<T, single_read_multiple_write>
		{
		private:
			std::atomic_int lasti;
			int readi;
			int writei;
			std::bitset<3> writebits;
			utility::static_storage<3, T> buffer_;
			utility::spinlock writelock;

		public:
			ExchangeQueue() :
				lasti(0),
				readi(1),
				writei(2)
			{}

		public:
			/**
			 * \param[out] item Where to write on success.
			 * \return True on success, false otherwise.
			 */
			bool try_pop(T & item)
			{
				readi = lasti.exchange(readi, std::memory_order_acquire);
				if (writebits.test(readi))
				{
					using utility::iter_move;
					item = iter_move(buffer_.begin() + readi);
					writebits.reset(readi);
					buffer_.destruct_at(buffer_.begin() + readi);
					return true;
				}
				return false;
			}
			/**
			 * \param[in] ps The thing to push.
			 * \return Always true.
			 */
			template <typename ...Ps>
			bool try_push(Ps && ...ps)
			{
				std::lock_guard<utility::spinlock> lock{this->writelock};

				writebits.set(writei);
				buffer_.construct_at(buffer_.begin() + writei, std::forward<Ps>(ps)...);
				writei = lasti.exchange(writei, std::memory_order_acq_rel); // release?
				// remove any unread item
				if (writebits.test(writei))
				{
					buffer_.destruct_at(buffer_.begin() + writei);
				}
				return true;
			}
		};

		template <typename T>
		using ExchangeQueueSRSW = ExchangeQueue<T, single_read_single_write>;
		template <typename T>
		using ExchangeQueueSRMW = ExchangeQueue<T, single_read_multiple_write>;
	}
}

#endif /* CORE_CONTAINER_EXCHANGEQUEUE_HPP */
