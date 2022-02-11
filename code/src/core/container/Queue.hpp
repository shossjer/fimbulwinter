
#pragma once

#include "core/debug.hpp"

#include "utility/aggregation_allocator.hpp"
#include "utility/concepts.hpp"
#include "utility/storage.hpp"

#include <atomic>
#include <cstring>

namespace utility
{
	template <typename T>
	T wrap_once(T value, T max)
	{
		return value < max ? value : value - max;
	}

	template <typename T>
	T wrap_reset(T value, T max)
	{
		return value < max ? value : T{};
	}
}

namespace core
{
	namespace container
	{
		namespace detail
		{
			template <typename StorageTraits, bool = StorageTraits::static_capacity::value>
			struct PageQueueHeader
			{
				using this_type = PageQueueHeader<StorageTraits, true>;

				this_type * previous_;

				std::atomic_size_t readi_;
				std::atomic_size_t readendi_;
				std::atomic_size_t writei_;

				PageQueueHeader()
					: previous_(this)
					, readi_{0}
					, readendi_{0}
					, writei_{std::size_t(-1)} // fake being full
				{}
				PageQueueHeader(this_type & previous, std::size_t debug_expression(capacity))
					: previous_(&previous)
					, readi_{0}
					, readendi_{0}
					, writei_{0}
				{
					debug_assert(capacity <= StorageTraits::capacity_value);
				}

				constexpr std::size_t capacity() const { return StorageTraits::capacity_value; }
			};
			template <typename StorageTraits>
			struct PageQueueHeader<StorageTraits, false /*static_capacity*/>
			{
				using this_type = PageQueueHeader<StorageTraits, false>;

				this_type * previous_;

				std::atomic_size_t readi_;
				std::atomic_size_t readendi_;
				std::atomic_size_t writei_;

				std::size_t capacity_;

				PageQueueHeader()
					: previous_(this)
					, readi_{0}
					, readendi_{0}
					, writei_{0}
					, capacity_(0)
				{}
				PageQueueHeader(this_type & previous, std::size_t capacity)
					: previous_(&previous)
					, readi_{0}
					, readendi_{0}
					, writei_{0}
					, capacity_(capacity)
				{}

				std::size_t capacity() const { return capacity_; }
			};

			template <typename StorageTraits>
			struct PageQueueCommon
			{
				static PageQueueHeader<StorageTraits> empty_header;
			};

			template <typename StorageTraits>
			PageQueueHeader<StorageTraits> PageQueueCommon<StorageTraits>::empty_header;
			// todo: templatify on bool is enough

			template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
			struct SimpleQueueData_static_capacity
			{
				std::atomic_size_t readi_;
				std::atomic_size_t readendi_;
				std::atomic_size_t writei_;

				Storage storage_;

				SimpleQueueData_static_capacity()
					: SimpleQueueData_static_capacity(utility::storage_traits<Storage>::capacity_value)
				{}
				explicit SimpleQueueData_static_capacity(std::size_t capacity)
					: readi_{0}
					, readendi_{0}
					, writei_{0}
				{
					fiw_unused(debug_verify(storage_.allocate(capacity)));
				}

				constexpr std::size_t capacity() const { return utility::storage_traits<Storage>::capacity_value; }
			};
			template <typename Storage>
			struct SimpleQueueData_static_capacity<Storage, false /*static_capacity*/>
			{
				std::atomic_size_t readi_;
				std::atomic_size_t readendi_;
				std::atomic_size_t writei_;

				std::size_t capacity_;

				Storage storage_;

				explicit SimpleQueueData_static_capacity(std::size_t capacity)
					: readi_{0}
					, readendi_{0}
					, writei_{0}
					, capacity_(capacity)
				{
					fiw_unused(debug_verify(storage_.allocate(capacity)));
				}

				std::size_t capacity() const { return capacity_; }
			};

			template <typename Storage, bool = utility::storage_is_trivially_destructible<Storage>::value>
			struct SimpleQueueData_trivially_destructible
				: SimpleQueueData_static_capacity<Storage>
			{
				using base_type = SimpleQueueData_static_capacity<Storage>;
				using this_type = SimpleQueueData_trivially_destructible<Storage, true>;

				using base_type::base_type;
			};
			template <typename Storage>
			struct SimpleQueueData_trivially_destructible<Storage, false /*trivially destructible*/>
				: SimpleQueueData_static_capacity<Storage>
			{
				using base_type = SimpleQueueData_static_capacity<Storage>;
				using this_type = SimpleQueueData_trivially_destructible<Storage, false>;

				using base_type::base_type;

				~SimpleQueueData_trivially_destructible()
				{
					const std::size_t readi = this->readi_.load(std::memory_order_relaxed);
					const std::size_t barrier = this->readendi_.load(std::memory_order_acquire);
					debug_assert(barrier == this->writei_.load(std::memory_order_relaxed));

					if (barrier < readi)
					{
						this->storage_.destruct_range(this->storage_.begin(this->capacity()), this->storage_.begin(this->capacity()) + barrier);
						this->storage_.destruct_range(this->storage_.begin(this->capacity()) + readi, this->storage_.begin(this->capacity()) + this->capacity());
					}
					else
					{
						this->storage_.destruct_range(this->storage_.begin(this->capacity()) + readi, this->storage_.begin(this->capacity()) + barrier);
					}
					this->storage_.deallocate(this->capacity());
				}
				SimpleQueueData_trivially_destructible() = default;
				SimpleQueueData_trivially_destructible(const this_type &) = default;
				SimpleQueueData_trivially_destructible(this_type &&) = default;
				this_type & operator = (const this_type &) = default;
				this_type & operator = (this_type &&) = default;
			};

			template <typename Storage>
			using SimpleQueueData = SimpleQueueData_trivially_destructible<Storage>;

			template <typename Header>
			bool checkout_write_slot(Header & header, std::size_t & slot, std::size_t & next)
			{
				slot = header.writei_.load(std::memory_order_relaxed);

				do
				{
					const std::size_t readi = header.readi_.load(std::memory_order_relaxed);

					next = utility::wrap_reset(slot + 1, header.capacity());
					if (readi == next)
						return false; // full
				}
				while (!header.writei_.compare_exchange_weak(slot, next, std::memory_order_relaxed));

				return true;
			}

			template <typename Header>
			void checkin_write_slot(Header & header, std::size_t slot, std::size_t next)
			{
				std::size_t barrier;
				do
				{
					barrier = slot;
				}
				while (!header.readendi_.compare_exchange_weak(barrier, next, std::memory_order_release, std::memory_order_relaxed));
			}
		}

		template <typename Storage>
		class PageQueue
		{
			using ReservationStrategy = utility::reserve_power_of_two<Storage>;

		public:
			using value_type = typename Storage::value_type;
			using reference = typename Storage::reference;
			using const_reference = typename Storage::const_reference;
			using rvalue_reference = typename Storage::rvalue_reference;
		private:
			using storage_traits = utility::storage_traits<Storage>;

			template <typename ...Ts>
			using page_allocator = utility::aggregation_allocator<storage_traits::template allocator_type, detail::PageQueueHeader<storage_traits>, Ts...>;
			using allocator_type = mpl::apply<page_allocator, typename storage_traits::type_list>;
			using allocator_traits = std::allocator_traits<allocator_type>;

			using Header = typename allocator_type::value_type;

		private:
			struct empty_allocator_hack : allocator_type
			{
				std::atomic<Header *> write_header;

				~empty_allocator_hack()
				{
					for (Header * header = write_header.load(std::memory_order_acquire); header != &detail::PageQueueCommon<storage_traits>::empty_header;)
					{
						Header * const previous = header->previous_;

						const auto capacity = header->capacity();
						allocator_traits::destroy(*this, header);
						allocator_traits::deallocate(*this, header, capacity);

						header = previous;
					}
				}
				empty_allocator_hack()
					: write_header{&detail::PageQueueCommon<storage_traits>::empty_header}
				{}
				empty_allocator_hack(std::size_t capacity)
					: write_header{}
				{
					Header * const header = allocator_traits::allocate(*this, capacity);
					debug_assert(header);

					allocator_traits::construct(*this, header, detail::PageQueueCommon<storage_traits>::empty_header, capacity);

					write_header.store(header, std::memory_order_release);
				}
			} impl_;

		public:
			PageQueue() = default;
			PageQueue(std::size_t capacity)
				: impl_(ReservationStrategy{}(capacity))
			{}

		public:
			template <typename ...Ps>
			bool try_emplace(Ps && ...ps)
			{
				Header * header = write_header().load(std::memory_order_acquire);

				std::size_t slot;
				std::size_t next;
				while (!detail::checkout_write_slot(*header, slot, next))
				{
					const auto new_capacity = ReservationStrategy{}(header->capacity() + 1);
					Header * const new_header = allocator_traits::allocate(allocator(), new_capacity);
					if (!new_header)
						return false;

					allocator_traits::construct(allocator(), new_header, *header, new_capacity);

					if (write_header().compare_exchange_strong(header, new_header, std::memory_order_release, std::memory_order_relaxed))
					{
						header = new_header;
					}
					else
					{
						allocator_traits::destroy(allocator(), new_header);
						allocator_traits::deallocate(allocator(), new_header, new_capacity);
					}
				}

				auto p = allocator().address(header, header->capacity()) + slot;
				allocator().construct(p, std::forward<Ps>(ps)...);

				detail::checkin_write_slot(*header, slot, next);

				return true;
			}

			bool try_pop(reference value)
			{
				Header * read_header = write_header().load(std::memory_order_acquire);

				if (read_header->previous_ != &empty_header())
				{
					Header * next_header;
					do
					{
						next_header = read_header;
						read_header = read_header->previous_;
					}
					while (read_header->previous_ != &empty_header());

					const std::size_t slot = read_header->readi_.load(std::memory_order_relaxed);
					const std::size_t writei = read_header->writei_.load(std::memory_order_relaxed);
					if (slot == writei)
					{
						next_header->previous_ = &empty_header();

						const auto capacity = read_header->capacity();
						allocator_traits::destroy(allocator(), read_header);
						allocator_traits::deallocate(allocator(), read_header, capacity);

						read_header = next_header;
					}
				}

				const std::size_t slot = read_header->readi_.load(std::memory_order_relaxed);
				const std::size_t barrier = read_header->readendi_.load(std::memory_order_relaxed);
				if (slot == barrier)
					return false;

				std::atomic_thread_fence(std::memory_order_acquire);

				using utility::iter_move;
				auto p = allocator().address(read_header, read_header->capacity()) + slot;
				value = iter_move(p);
				// allocator_traits::destroy(allocator(), p);
				allocator().destroy(p);

				const std::size_t next = utility::wrap_reset(slot + 1, read_header->capacity());
				read_header->readi_.store(next, std::memory_order_relaxed);

				return true;
			}

			bool try_push(const_reference value)
			{
				return try_emplace(value);
			}

			bool try_push(rvalue_reference value)
			{
				return try_emplace(std::move(value));
			}

			// if `P` can be used to call both versions of `try_push` then
			// prefer the rvalue one
			template <typename P,
			          REQUIRES((std::is_constructible<const_reference, P &&>::value)),
			          REQUIRES((std::is_constructible<rvalue_reference, P &&>::value))>
			bool try_push(P && value)
			{
				return try_push(rvalue_reference(std::forward<P>(value)));
			}
		private:
			allocator_type & allocator() { return impl_; }
			const allocator_type & allocator() const { return impl_; }

			std::atomic<Header *> & write_header() { return impl_.write_header; }
			const std::atomic<Header *> & write_header() const { return impl_.write_header; }

			static Header & empty_header() { return detail::PageQueueCommon<storage_traits>::empty_header; }
		};

		template <typename Storage>
		class SimpleQueue
		{
			using ReservationStrategy = utility::reserve_power_of_two<Storage>;

		public:
			using value_type = typename Storage::value_type;
			using reference = typename Storage::reference;
			using const_reference = typename Storage::const_reference;
			using rvalue_reference = typename Storage::rvalue_reference;
		private:
			using storage_traits = utility::storage_traits<Storage>;

		private:
			detail::SimpleQueueData<Storage> data;

		public:
			SimpleQueue() = default;

			explicit SimpleQueue(std::size_t size)
				: data(ReservationStrategy{}(size))
			{}

		public:
			constexpr std::size_t capacity() const { return data.capacity() - 1; }

			template <typename ...Ps>
			bool try_emplace(Ps && ...ps)
			{
				std::size_t slot;
				std::size_t next;
				if (!detail::checkout_write_slot(data, slot, next))
					return false;

				data.storage_.construct_at_(data.storage_.begin(data.capacity()) + slot, std::forward<Ps>(ps)...);

				detail::checkin_write_slot(data, slot, next);

				return true;
			}

			bool try_pop(reference value)
			{
				const std::size_t slot = data.readi_.load(std::memory_order_relaxed);
				const std::size_t barrier = data.readendi_.load(std::memory_order_relaxed);
				if (slot == barrier)
					return false;

				std::atomic_thread_fence(std::memory_order_acquire);

				using utility::iter_move;
				value = iter_move(data.storage_.data(data.storage_.begin(data.capacity())) + slot);
				data.storage_.destruct_at(data.storage_.begin(data.capacity()) + slot);

				const std::size_t next = utility::wrap_reset(slot + 1, data.capacity());
				data.readi_.store(next, std::memory_order_relaxed);

				return true;
			}

			bool try_push(const_reference value)
			{
				return try_emplace(value);
			}

			bool try_push(rvalue_reference value)
			{
				return try_emplace(std::move(value));
			}

			// if `P` can be used to call both versions of `try_push` then
			// prefer the rvalue one
			template <typename P,
			          REQUIRES((std::is_constructible<const_reference, P &&>::value)),
			          REQUIRES((std::is_constructible<rvalue_reference, P &&>::value))>
			bool try_push(P && value)
			{
				return try_push(rvalue_reference(std::forward<P>(value)));
			}
		};
	}
}
