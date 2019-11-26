
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

				this_type * previous;

				std::atomic_size_t readi;
				std::atomic_size_t readendi;
				std::atomic_size_t writei;

				PageQueueHeader()
					: previous(this)
					, readi{0}
					, readendi{0}
					, writei{std::size_t(-1)} // fake being full
				{}
				PageQueueHeader(this_type & previous, std::size_t capacity_)
					: previous(&previous)
					, readi{0}
					, readendi{0}
					, writei{0}
				{
					debug_assert(capacity_ <= StorageTraits::capacity_value);
				}

				constexpr std::size_t capacity() const { return StorageTraits::capacity_value; }
			};
			template <typename StorageTraits>
			struct PageQueueHeader<StorageTraits, false /*static_capacity*/>
			{
				using this_type = PageQueueHeader<StorageTraits, false>;

				this_type * previous;

				std::atomic_size_t readi;
				std::atomic_size_t readendi;
				std::atomic_size_t writei;

				std::size_t capacity_;

				PageQueueHeader()
					: previous(this)
					, readi{0}
					, readendi{0}
					, writei{0}
					, capacity_(0)
				{}
				PageQueueHeader(this_type & previous, std::size_t capacity_)
					: previous(&previous)
					, readi{0}
					, readendi{0}
					, writei{0}
					, capacity_(capacity_)
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
				std::atomic_size_t readi;
				std::atomic_size_t readendi;
				std::atomic_size_t writei;

				Storage storage_;

				SimpleQueueData_static_capacity()
					: SimpleQueueData_static_capacity(utility::storage_traits<Storage>::capacity_value)
				{}
				SimpleQueueData_static_capacity(std::size_t capacity_)
					: readi{0}
					, readendi{0}
					, writei{0}
				{
					debug_verify(storage_.allocate(capacity_));
				}

				constexpr std::size_t capacity() const { return utility::storage_traits<Storage>::capacity_value; }
			};
			template <typename Storage>
			struct SimpleQueueData_static_capacity<Storage, false /*static_capacity*/>
			{
				std::atomic_size_t readi;
				std::atomic_size_t readendi;
				std::atomic_size_t writei;

				std::size_t capacity_;

				Storage storage_;

				SimpleQueueData_static_capacity(std::size_t capacity_)
					: readi{0}
					, readendi{0}
					, writei{0}
					, capacity_(capacity_)
				{
					debug_verify(storage_.allocate(capacity_));
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
					const std::size_t readi = this->readi.load(std::memory_order_relaxed);
					const std::size_t barrier = this->readendi.load(std::memory_order_acquire);
					debug_assert(barrier == this->writei.load(std::memory_order_relaxed));

					auto sections = this->storage_.sections(this->capacity());
					if (barrier < readi)
					{
						sections.destruct_range(0, barrier);
						sections.destruct_range(readi, this->capacity());
					}
					else
					{
						sections.destruct_range(readi, barrier);
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
				slot = header.writei.load(std::memory_order_relaxed);

				do
				{
					const std::size_t readi = header.readi.load(std::memory_order_relaxed);

					next = utility::wrap_reset(slot + 1, header.capacity());
					if (readi == next)
						return false; // full
				}
				while (!header.writei.compare_exchange_weak(slot, next, std::memory_order_relaxed));

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
				while (!header.readendi.compare_exchange_weak(barrier, next, std::memory_order_release, std::memory_order_relaxed));
			}
		}

		template <typename Storage>
		class PageQueue
		{
		public:
			using value_type = typename Storage::value_type;
			using reference = typename Storage::reference;
			using const_reference = typename Storage::const_reference;
			using rvalue_reference = typename Storage::rvalue_reference;
		private:
			using storage_traits = utility::storage_traits<Storage>;

			template <typename ...Ts>
			using page_allocator = utility::aggregation_allocator<storage_traits::template allocator_type, detail::PageQueueHeader<storage_traits>, Ts...>;
			using allocator_type = mpl::apply<page_allocator, typename Storage::value_types>;
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
						Header * const previous = header->previous;

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
				: impl_(storage_traits::capacity_for(capacity))
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
					const auto new_capacity = storage_traits::grow(header->capacity(), 0);
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

				if (read_header->previous != &empty_header())
				{
					Header * next_header;
					do
					{
						next_header = read_header;
						read_header = read_header->previous;
					}
					while (read_header->previous != &empty_header());

					const std::size_t slot = read_header->readi.load(std::memory_order_relaxed);
					const std::size_t writei = read_header->writei.load(std::memory_order_relaxed);
					if (slot == writei)
					{
						next_header->previous = &empty_header();

						const auto capacity = read_header->capacity();
						allocator_traits::destroy(allocator(), read_header);
						allocator_traits::deallocate(allocator(), read_header, capacity);

						read_header = next_header;
					}
				}

				const std::size_t slot = read_header->readi.load(std::memory_order_relaxed);
				const std::size_t barrier = read_header->readendi.load(std::memory_order_relaxed);
				if (slot == barrier)
					return false;

				std::atomic_thread_fence(std::memory_order_acquire);

				using utility::iter_move;
				auto p = allocator().address(read_header, read_header->capacity()) + slot;
				value = iter_move(p);
				// allocator_traits::destroy(allocator(), p);
				allocator().destroy(p);

				const std::size_t next = utility::wrap_reset(slot + 1, read_header->capacity());
				read_header->readi.store(next, std::memory_order_relaxed);

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
			SimpleQueue(std::size_t capacity)
				: data(storage_traits::capacity_for(capacity))
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

				data.storage_.sections(data.capacity()).construct_at(slot, std::forward<Ps>(ps)...);

				detail::checkin_write_slot(data, slot, next);

				return true;
			}

			bool try_pop(reference value)
			{
				const std::size_t slot = data.readi.load(std::memory_order_relaxed);
				const std::size_t barrier = data.readendi.load(std::memory_order_relaxed);
				if (slot == barrier)
					return false;

				std::atomic_thread_fence(std::memory_order_acquire);

				using utility::iter_move;
				auto sections = data.storage_.sections(data.capacity());
				value = iter_move(sections.data() + slot);
				sections.destruct_at(slot);

				const std::size_t next = utility::wrap_reset(slot + 1, data.capacity());
				data.readi.store(next, std::memory_order_relaxed);

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
