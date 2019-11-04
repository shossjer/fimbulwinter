
#ifndef UTILITY_STORAGE_HPP
#define UTILITY_STORAGE_HPP

#include "config.h"

#include "utility/aggregation_allocator.hpp"
#include "utility/heap_allocator.hpp"
#include "utility/iterator.hpp"
#include "utility/null_allocator.hpp"
#include "utility/storing.hpp"
#include "utility/tuple.hpp"

#include <array>
#include <cassert>
#include <cstring>

namespace utility
{
	template <typename Storage, std::size_t I>
	class section
	{
	public:
		using value_type = typename Storage::template value_type<I>;

		using storing_type = typename Storage::template storing_type<value_type>;
		using storing_trivially_copyable = std::is_trivially_copyable<storing_type>;
		using storing_trivially_destructible = std::is_trivially_destructible<storing_type>;

		template <typename InputIt>
		using can_memcpy = mpl::conjunction<storing_trivially_copyable,
		                                    utility::is_contiguous_iterator<InputIt>>;

	private:
		Storage * storage_;
		storing_type * data_;

	public:
		section(Storage & storage_, storing_type * data_)
			: storage_(&storage_)
			, data_(data_)
		{}

	public:
		value_type & operator [] (std::ptrdiff_t index)
		{
			return storage_->value_at(data_ + index);
		}
		const value_type & operator [] (std::ptrdiff_t index) const
		{
			return storage_->value_at(data_ + index);
		}

		template <typename ...Ps>
		void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			for (; begin != end; begin++)
			{
				storage_->construct_at(data_, begin, ps...);
			}
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(can_memcpy<InputIt>{}, index, begin, end);
		}

		template <typename ...Ps>
		value_type & construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			return storage_->construct_at(data_, index, std::forward<Ps>(ps)...);
		}

		template <typename InputIt,
		          REQUIRES((can_memcpy<InputIt>::value))>
		void memcpy_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(mpl::true_type{}, index, begin, end);
		}

		value_type * data()
		{
			return storage_->data(data_);
		}
		const value_type * data() const
		{
			return storage_->data(data_);
		}

		void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			for (; begin != end; begin++)
			{
				storage_->destruct_at(data_, begin);
			}
		}

		void destruct_at(std::size_t index)
		{
			storage_->destruct_at(data_, index);
		}

		std::ptrdiff_t index_of(const value_type & x) const
		{
			return storage_->index_of(data_, x);
		}
	private:
		void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, const value_type * begin, const value_type * end)
		{
			assert(begin <= end);
			std::memcpy(storage_->data(data_) + index, begin, (end - begin) * sizeof(value_type));
		}
		void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, value_type * begin, value_type * end)
		{
			construct_range_impl(mpl::true_type{}, index, const_cast<const value_type *>(begin), const_cast<const value_type *>(end));
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
			construct_range_impl(mpl::true_type{}, index, begin.base(), end.base());
		}
		template <typename InputIt>
		void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(mpl::true_type{}, index, begin.base(), end.base());
		}

		template <typename InputIt>
		void construct_range_impl(mpl::false_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			for (; begin != end; index++, ++begin)
			{
				storage_->construct_at(data_, index, *begin);
			}
		}
	};

	template <typename Storage, std::size_t I>
	class const_section
	{
	public:
		using value_type = typename Storage::template value_type<I>;

		using storing_type = typename Storage::template storing_type<value_type>;

	private:
		const Storage * storage_;
		const storing_type * data_;

	public:
		const_section(const Storage & storage_, const storing_type * data_)
			: storage_(&storage_)
			, data_(data_)
		{}

	public:
		const value_type & operator [] (std::ptrdiff_t index) const
		{
			return storage_->value_at(data_ + index);
		}

		const value_type * data() const
		{
			return storage_->data(data_);
		}

		std::ptrdiff_t index_of(const value_type & x) const
		{
			return storage_->index_of(data_, x);
		}
	};

	namespace detail
	{
		template <std::size_t Capacity, typename ...Ts>
		class static_storage_impl
		{
		private:
			using this_type = static_storage_impl<Capacity, Ts...>;

		public:
			template <std::size_t I>
			using value_type = mpl::type_at<I, Ts...>;

			template <typename T>
			using storing_type = utility::storing<T>;
			using storing_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<storing_type<Ts>>...>;
			using storing_trivially_destructible = mpl::conjunction<std::is_trivially_destructible<storing_type<Ts>>...>;

			using allocator_type = utility::null_allocator<char>;

			static constexpr std::size_t rank = sizeof...(Ts);

		private:
			utility::tuple<std::array<storing_type<Ts>, Capacity>...> arrays;

		public:
			bool allocate(std::size_t capacity)
			{
				return capacity <= Capacity; // todo ==
			}

			void deallocate(std::size_t capacity)
			{
				assert(capacity <= Capacity); // todo ==
			}

			constexpr std::size_t max_size() const { return Capacity; }

			template <typename StoringType, typename ...Ps>
			typename StoringType::value_type & construct_at(StoringType * data_, std::ptrdiff_t index, Ps && ...ps)
			{
				assert((0 <= index && index < Capacity));
				return data_[index].construct(std::forward<Ps>(ps)...);
			}

			template <typename StoringType>
			void destruct_at(StoringType * data_, std::ptrdiff_t index)
			{
				assert((0 <= index && index < Capacity));
				data_[index].destruct();
			}

			template <typename StoringType>
			typename StoringType::value_type * data(StoringType * data_)
			{
				return &data_->value;
			}
			template <typename StoringType>
			const typename StoringType::value_type * data(const StoringType * data_) const
			{
				return &data_->value;
			}

			template <typename StoringType>
			std::ptrdiff_t index_of(const StoringType * data_, const typename StoringType::value_type & x) const
			{
				// x is pointer interconvertible with storing_t, since
				// storing_t is a union containing a value_type member
				return reinterpret_cast<const StoringType *>(std::addressof(x)) - data_;
			}

			template <typename StoringType>
			typename StoringType::value_type & value_at(StoringType * p)
			{
				return p->value;
			}
			template <typename StoringType>
			const typename StoringType::value_type & value_at(const StoringType * p) const
			{
				return p->value;
			}

			template <std::size_t I>
			utility::section<this_type, I> section(mpl::index_constant<I>)
			{
				return utility::section<this_type, I>(*this, get<I>(arrays).data());
			}
			template <std::size_t I>
			utility::const_section<this_type, I> section(mpl::index_constant<I>) const
			{
				return utility::const_section<this_type, I>(*this, get<I>(arrays).data());
			}
			template <std::size_t I>
			utility::section<this_type, I> section(mpl::index_constant<I>, std::size_t /*capacity*/)
			{
				return section(mpl::index_constant<I>{});
			}
			template <std::size_t I>
			utility::const_section<this_type, I> section(mpl::index_constant<I>, std::size_t /*capacity*/) const
			{
				return section(mpl::index_constant<I>{});
			}
		};

		template <template <typename> class Allocator, typename ...Ts>
		class dynamic_storage_impl
		{
		private:
			using this_type = dynamic_storage_impl<Allocator, Ts...>;

		public:
			template <std::size_t I>
			using value_type = mpl::type_at<I, Ts...>;

			template <typename T>
			using storing_type = T;
			using storing_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<storing_type<Ts>>...>;
			using storing_trivially_destructible = mpl::conjunction<std::is_trivially_destructible<storing_type<Ts>>...>;

			using allocator_type = utility::aggregation_allocator<Allocator, void, Ts...>;

			static constexpr std::size_t rank = sizeof...(Ts);
		private:
			using allocator_traits = std::allocator_traits<allocator_type>;

		private:
			struct empty_allocator_hack : allocator_type
			{
#if MODE_DEBUG
				void * storage_ = nullptr;

				~empty_allocator_hack()
				{
					assert(!storage_);
				}
				empty_allocator_hack() = default;
				empty_allocator_hack(empty_allocator_hack && other)
					: storage_(std::exchange(other.storage_, nullptr))
				{}
				empty_allocator_hack & operator = (empty_allocator_hack && other)
				{
					assert(!storage_);

					storage_ = std::exchange(other.storage_, nullptr);

					return *this;
				}
#else
				void * storage_;

				empty_allocator_hack() = default;
				empty_allocator_hack(empty_allocator_hack && other) = default;
				empty_allocator_hack & operator = (empty_allocator_hack && other) = default;
#endif
			} impl_;

		public:
			bool allocate(std::size_t capacity)
			{
#if MODE_DEBUG
				assert(!storage());
#endif
				storage() = allocator_traits::allocate(allocator(), capacity);
				return storage() != nullptr;
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

			template <typename T, typename ...Ps>
			T & construct_at(T * data_, std::ptrdiff_t index, Ps && ...ps)
			{
#if MODE_DEBUG
				assert(storage());
#endif
				allocator_traits::construct(allocator(), data_ + index, std::forward<Ps>(ps)...);
				return data_[index];
			}

			template <typename T>
			void destruct_at(T * data_, std::ptrdiff_t index)
			{
#if MODE_DEBUG
				assert(storage());
#endif
				allocator_traits::destroy(allocator(), data_ + index);
			}

			template <typename T>
			T * data(T * data_)
			{
				return data_;
			}
			template <typename T>
			const T * data(const T * data_) const
			{
				return data_;
			}

			template <typename T>
			std::ptrdiff_t index_of(const T * data_, const T & x) const
			{
#if MODE_DEBUG
				assert(storage());
#endif
				return std::addressof(x) - data_;
			}

			template <typename T>
			T & value_at(T * p)
			{
#if MODE_DEBUG
				assert(storage());
#endif
				return *p;
			}
			template <typename T>
			const T & value_at(const T * p) const
			{
#if MODE_DEBUG
				assert(storage());
#endif
				return *p;
			}

			template <std::size_t I>
			utility::section<this_type, I> section(mpl::index_constant<I>, std::size_t capacity)
			{
#if MODE_DEBUG
				assert(storage() || capacity == 0);
#endif
				// there are times in which we will call allocator address
				// with a garbage storage, but maybe that is okay as long
				// as we never try to use that address?
				return utility::section<this_type, I>(*this, allocator().template address<I>(storage(), capacity));
			}
			template <std::size_t I>
			utility::const_section<this_type, I> section(mpl::index_constant<I>, std::size_t capacity) const
			{
#if MODE_DEBUG
				assert(storage() || capacity == 0);
#endif
				// ditto
				return utility::const_section<this_type, I>(*this, allocator().template address<I>(storage(), capacity));
			}
			utility::section<this_type, 0> section(mpl::index_constant<0>)
			{
				return section(mpl::index_constant<0>{}, 0);
			}
			utility::const_section<this_type, 0> section(mpl::index_constant<0>) const
			{
				return section(mpl::index_constant<0>{}, 0);
			}
		protected:
			allocator_type & allocator() { return impl_; }
			const allocator_type & allocator() const { return impl_; }

			void * & storage() { return impl_.storage_; }
			const void * storage() const { return impl_.storage_; }
		};
	}

	template <typename StorageImpl, int = StorageImpl::rank>
	class basic_storage
		: public StorageImpl
	{
	public:
		template <std::size_t I>
		using value_type = typename StorageImpl::template value_type<I>;
		template <typename T>
		using storing_type = typename StorageImpl::template storing_type<T>;
	};
	template <typename StorageImpl>
	class basic_storage<StorageImpl, 1>
		: public StorageImpl
	{
	public:
		template <std::size_t I>
		using value_type = typename StorageImpl::template value_type<I>;
		template <typename T>
		using storing_type = typename StorageImpl::template storing_type<T>;
	private:
		using single_value_type = value_type<0>;

		using single_storing_type = storing_type<single_value_type>;
		using single_storing_trivially_copyable = std::is_trivially_copyable<single_storing_type>;
		using single_storing_trivially_destructible = std::is_trivially_destructible<single_storing_type>;

		template <typename InputIt>
		using can_memcpy = mpl::conjunction<single_storing_trivially_copyable,
		                                    utility::is_contiguous_iterator<InputIt>>;

	public:
		single_value_type & operator [] (std::ptrdiff_t index)
		{
			return single_section()[index];
		}
		const single_value_type & operator [] (std::ptrdiff_t index) const
		{
			return single_section()[index];
		}

		template <typename ...Ps>
		void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			single_section().construct_fill(begin, end, std::forward<Ps>(ps)...);
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			single_section().construct_range(index, begin, end);
		}

		using StorageImpl::construct_at;
		template <typename ...Ps>
		single_value_type & construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			return single_section().construct_at(index, std::forward<Ps>(ps)...);
		}

		template <typename InputIt,
		          REQUIRES((can_memcpy<InputIt>::value))>
		void memcpy_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			single_section().memcpy_range(index, begin, end);
		}

		using StorageImpl::data;
		single_value_type * data()
		{
			return single_section().data();
		}
		const single_value_type * data() const
		{
			return single_section().data();
		}

		void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			for (; begin != end; begin++)
			{
				single_section().destruct_at(begin);
			}
		}

		using StorageImpl::destruct_at;
		void destruct_at(std::size_t index)
		{
			single_section().destruct_at(index);
		}

		using StorageImpl::index_of;
		std::ptrdiff_t index_of(const single_value_type & x)
		{
			return single_section().index_of(x);
		}
	private:
		StorageImpl & base() { return static_cast<StorageImpl &>(*this); }
		const StorageImpl & base() const { return static_cast<const StorageImpl &>(*this); }

		section<StorageImpl, 0> single_section() { return base().section(mpl::index_constant<0>{}); }
		const_section<StorageImpl, 0> single_section() const { return base().section(mpl::index_constant<0>{}); }
	};

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_storage = basic_storage<detail::dynamic_storage_impl<Allocator, Ts...>>;
	template <std::size_t Capacity, typename ...Ts>
	using static_storage = basic_storage<detail::static_storage_impl<Capacity, Ts...>>;

	template <typename ...Ts>
	using heap_storage = dynamic_storage<utility::heap_allocator, Ts...>;

	template <typename Storage>
	struct storage_traits;
	template <template <typename> class Allocator, typename ...Ts>
	struct storage_traits<dynamic_storage<Allocator, Ts...>>
	{
		using allocator_type = typename dynamic_storage<Allocator, Ts...>::allocator_type;
		template <typename ...Us>
		using storage_type = dynamic_storage<Allocator, Us...>;

		using static_capacity = mpl::false_type;
		using trivial_allocate = mpl::false_type;
		using trivial_deallocate = mpl::false_type;
		using moves_allocation = mpl::true_type;

		static std::size_t grow(std::size_t capacity, std::size_t amount)
		{
			assert(amount <= 8 || amount <= capacity);
			return capacity < 8 ? 8 : capacity * 2;
		}

		static std::size_t capacity_for(std::size_t size)
		{
			return size;
		}
	};
	template <std::size_t Capacity, typename ...Ts>
	struct storage_traits<static_storage<Capacity, Ts...>>
	{
		using allocator_type = typename static_storage<Capacity, Ts...>::allocator_type;
		template <typename ...Us>
		using storage_type = static_storage<Capacity, Us...>;

		using static_capacity = mpl::true_type;
		using trivial_allocate = mpl::true_type;
		using trivial_deallocate = mpl::true_type;
		using moves_allocation = mpl::false_type;

		static constexpr std::size_t capacity_value = Capacity;

		static constexpr std::size_t grow(std::size_t /*capacity*/, std::size_t /*amount*/)
		{
			return capacity_value;
		}

		static constexpr std::size_t capacity_for(std::size_t /*size*/)
		{
			return capacity_value;
		}
	};

	template <std::size_t Capacity>
	using static_storage_traits = storage_traits<static_storage<Capacity, int>>; // todo remove int?
	template <template <typename> class Allocator>
	using dynamic_storage_traits = storage_traits<dynamic_storage<Allocator, int>>; // todo remove int?
	using heap_storage_traits = dynamic_storage_traits<heap_allocator>;
	using null_storage_traits = dynamic_storage_traits<null_allocator>;

	template <typename Storage>
	using storage_is_trivially_destructible =
		mpl::conjunction<typename Storage::storing_trivially_destructible,
		                 typename storage_traits<Storage>::trivial_deallocate>;
	template <typename Storage>
	using storage_is_copy_constructible = std::is_copy_constructible<Storage>;
	template <typename Storage>
	using storage_is_copy_assignable = std::is_copy_assignable<Storage>;
	template <typename Storage>
	using storage_is_trivially_move_constructible =
		mpl::conjunction<std::is_move_constructible<Storage>,
		                 mpl::negation<typename storage_traits<Storage>::moves_allocation>>;
	template <typename Storage>
	using storage_is_trivially_move_assignable =
		mpl::conjunction<std::is_move_assignable<Storage>,
		                 mpl::negation<typename storage_traits<Storage>::moves_allocation>>;
}

#endif /* UTILITY_STORAGE_HPP */
