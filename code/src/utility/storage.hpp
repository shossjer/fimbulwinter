
#ifndef UTILITY_STORAGE_HPP
#define UTILITY_STORAGE_HPP

#include "config.h"
#include "heap_allocator.hpp"
#include "iterator.hpp"
#include "storing.hpp"
#include "utility.hpp"

#include <array>
#include <cassert>
#include <cstring>

namespace utility
{
	namespace detail
	{
		template <typename T, typename Storing, typename Derived>
		struct storage_helper
		{
			using value_type = T;

			using storing_t = Storing;
			using storing_trivially_copyable = std::is_trivially_copyable<storing_t>;
			using storing_trivially_destructible = std::is_trivially_destructible<storing_t>;

			template <typename InputIt>
			using can_memcpy = mpl::conjunction<storing_trivially_copyable,
			                                    utility::is_contiguous_iterator<InputIt>>;

			constexpr value_type & operator [] (std::ptrdiff_t index)
			{
				return derived().data()[index];
			}
			constexpr const value_type & operator [] (std::ptrdiff_t index) const
			{
				return derived().data()[index];
			}

			template <typename ...Ps>
			void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
			{
				for (; begin != end; begin++)
				{
					derived().construct_at(begin, ps...);
				}
			}
			template <typename InputIt>
			void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
			{
				construct_range_impl(can_memcpy<InputIt>{}, index, begin, end);
			}
			template <typename InputIt,
			          REQUIRES((can_memcpy<InputIt>::value))>
			void memcpy_range(std::ptrdiff_t index, InputIt begin, InputIt end)
			{
				construct_range_impl(mpl::true_type{}, index, begin, end);
			}
			void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
			{
				for (; begin != end; begin++)
				{
					derived().destruct_at(begin);
				}
			}
		private:
			void construct_range_impl(std::true_type /*can_memcpy*/, std::ptrdiff_t index, const value_type * begin, const value_type * end)
			{
				assert(begin < end);
				std::memcpy(derived().data() + index, begin, (end - begin) * sizeof(value_type));
			}
			template <typename InputIt>
			void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, std::move_iterator<InputIt> begin, std::move_iterator<InputIt> end)
			{
				construct_range_impl(mpl::true_type{}, index, begin.base(), end.base());
			}
			template <typename BidirectionalIt>
			void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, std::reverse_iterator<BidirectionalIt> begin, std::reverse_iterator<BidirectionalIt> end)
			{
				construct_range_impl(mpl::true_type{}, index, ++end.base(), ++begin.base());
			}
			template <typename InputIt>
			void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, utility::contiguous_iterator<InputIt> begin, utility::contiguous_iterator<InputIt> end)
			{
				// we are assuming here that the contiguous_iterator is
				// not wrapping an iterator wrapping another iterator, it
				// should be a terminating iterator as to speak
				construct_range_impl(mpl::true_type{}, index, &*begin, &*end);
			}
			template <typename InputIt>
			void construct_range_impl(mpl::false_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
			{
				for (; begin != end; index++, ++begin)
				{
					derived().construct_at(index, *begin);
				}
			}
			template <typename InputIt>
			void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
			{
				construct_range_impl(mpl::false_type{}, index, begin, end);
			}

			Derived & derived() { return static_cast<Derived &>(*this); }
			const Derived & derived() const { return static_cast<const Derived &>(*this); }
		};
	}

	template <typename T, std::size_t Capacity>
	class static_storage : public detail::storage_helper<T, storing<T>, static_storage<T, Capacity>>
	{
	private:
		using base_type = detail::storage_helper<T, storing<T>, static_storage<T, Capacity>>;

	public:
		using value_type = typename base_type::value_type;

		using storing_trivially_copyable = typename base_type::storing_trivially_copyable;
		using storing_trivially_destructible = typename base_type::storing_trivially_destructible;
	private:
		using storing_t = typename base_type::storing_t;

		static_assert(sizeof(storing_t) == sizeof(value_type), "");
		static_assert(alignof(storing_t) == alignof(value_type), "");

	public:
		std::array<storing_t, Capacity> storage_;

	public:
		void allocate(std::size_t capacity)
		{
			assert(capacity <= Capacity);
		}

		void deallocate(std::size_t capacity)
		{
			assert(capacity <= Capacity);
		}

		constexpr std::size_t max_size() const { return Capacity; }

		template <typename ...Ps>
		value_type & construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			assert((0 <= index && index < Capacity));
			return storage_[index].construct(std::forward<Ps>(ps)...);
		}

		void destruct_at(std::ptrdiff_t index)
		{
			assert((0 <= index && index < Capacity));
			storage_[index].destruct();
		}

		constexpr value_type * data() { return &storage_[0].value; }
		constexpr const value_type * data() const { return &storage_[0].value; }

		constexpr std::ptrdiff_t index_of(const value_type & x) const
		{
			// x is pointer interconvertible with storing_t, since
			// storing_t is a union containing a value_type member
			return reinterpret_cast<const storing_t *>(&x) - &storage_[0];
		}
	};

	template <typename T, template <typename> class Allocator>
	class dynamic_storage : public detail::storage_helper<T, T, dynamic_storage<T, Allocator>>
	{
	private:
		using base_type = detail::storage_helper<T, T, dynamic_storage<T, Allocator>>;

	public:
		using value_type = typename base_type::value_type;

		using storing_trivially_copyable = typename base_type::storing_trivially_copyable;
		using storing_trivially_destructible = typename base_type::storing_trivially_destructible;
	private:
		using storing_t = typename base_type::storing_t;

	public:
		using allocator_type = Allocator<storing_t>;
	private:
		using allocator_traits = std::allocator_traits<allocator_type>;

	private:
		struct empty_allocator_hack : allocator_type
		{
#if MODE_DEBUG
			storing_t * storage_ = nullptr;

			~empty_allocator_hack()
			{
				assert(!storage_);
			}
			empty_allocator_hack() = default;
			empty_allocator_hack(empty_allocator_hack && other)
				: storage_(other.storage_)
			{
				other.storage_ = nullptr;
			}
			empty_allocator_hack & operator = (empty_allocator_hack && other)
			{
				assert(!storage_);

				storage_ = std::exchange(other.storage_, nullptr);

				return *this;
			}
#else
			storing_t * storage_;

			empty_allocator_hack() = default;
			empty_allocator_hack(empty_allocator_hack && other) = default;
			empty_allocator_hack & operator = (empty_allocator_hack && other) = default;
#endif
		} impl_;

	public:
		void allocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(!storage());
#endif
			storage() = allocator_traits::allocate(allocator(), capacity);
		}

		void deallocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::deallocate(allocator(), storage(), capacity);
#if MODE_DEBUG
			storage() = nullptr;
#endif
		}

		constexpr std::size_t max_size() const { return allocator_traits::max_size(); }

		template <typename ...Ps>
		value_type & construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::construct(allocator(), storage() + index, std::forward<Ps>(ps)...);
			return storage()[index];
		}

		void destruct_at(std::ptrdiff_t index)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::destroy(allocator(), storage() + index);
		}

		value_type * data() { return storage() + 0; }
		const value_type * data() const { return storage() + 0; }

		std::ptrdiff_t index_of(const value_type & x) const { return &x - storage(); }
	private:
		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		storing_t * & storage() { return impl_.storage_; }
		const storing_t * storage() const { return impl_.storage_; }
	};

	template <typename T>
	using heap_storage = dynamic_storage<T, heap_allocator>;

	template <typename Storage>
	struct storage_traits;
	template <typename T, std::size_t Capacity>
	struct storage_traits<static_storage<T, Capacity>>
	{
		template <typename U>
		using storage_type = static_storage<U, Capacity>;

		using static_capacity = mpl::true_type;
		using trivial_allocate = mpl::true_type;
		using trivial_deallocate = mpl::true_type;

		static constexpr std::size_t capacity_value = Capacity;
	};
	template <typename T>
	struct storage_traits<heap_storage<T>>
	{
		template <typename U>
		using storage_type = heap_storage<U>;

		using static_capacity = mpl::false_type;
		using trivial_allocate = mpl::false_type;
		using trivial_deallocate = mpl::false_type;

		static std::size_t grow(std::size_t capacity)
		{
			return capacity < 8 ? 8 : capacity * 2;
		}
	};

	template <std::size_t Capacity>
	using static_storage_traits = storage_traits<static_storage<int, Capacity>>;
	template <template <typename> class Allocator>
	using dynamic_storage_traits = storage_traits<dynamic_storage<int, Allocator>>;
	using heap_storage_traits = dynamic_storage_traits<heap_allocator>;
}

#endif /* UTILITY_STORAGE_HPP */
