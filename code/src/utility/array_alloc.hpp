
#ifndef UTILITY_ARRAY_ALLOC_HPP
#define UTILITY_ARRAY_ALLOC_HPP

#include "property.hpp"
#include "storage.hpp"
#include "tuple.hpp"
#include "utility.hpp"

#include <cassert>

namespace utility
{
	namespace detail
	{
		template <typename StorageTraits>
		using array_alloc_is_trivially_destructible = typename StorageTraits::trivial_deallocate;
		template <typename StorageTraits>
		using array_alloc_is_trivially_copy_constructible = typename StorageTraits::trivial_allocate;
		template <typename StorageTraits>
		using array_alloc_is_trivially_copy_assignable =
			mpl::conjunction<typename StorageTraits::trivial_allocate,
			                 typename StorageTraits::trivial_deallocate>;

		template <typename StorageTraits, typename ...Ts>
		using array_alloc_is_copy_constructible =
			mpl::disjunction<mpl::conjunction<std::is_copy_constructible<typename StorageTraits::template storage_type<Ts>>...>,
			                 mpl::conjunction<typename StorageTraits::template storage_type<Ts>::storing_trivially_copyable...>>;
		template <typename StorageTraits, typename ...Ts>
		using array_alloc_is_copy_assignable =
			mpl::disjunction<mpl::conjunction<std::is_copy_assignable<typename StorageTraits::template storage_type<Ts>>...>,
			                 mpl::conjunction<typename StorageTraits::template storage_type<Ts>::storing_trivially_copyable...>>;

		struct array_alloc_helper
		{
			template <typename Tuple>
			static void allocate_storages(mpl::index_sequence<>, Tuple & storages, std::size_t capacity) {}
			template <std::size_t I, std::size_t ...Is, typename Tuple>
			static void allocate_storages(mpl::index_sequence<I, Is...>, Tuple & storages, std::size_t capacity)
			{
				get<I>(storages).allocate(capacity);
				allocate_storages(mpl::index_sequence<Is...>{}, storages, capacity);
			}

			template <typename Tuple>
			static void construct_range_storages(mpl::index_sequence<>, Tuple & dest, Tuple & src, std::ptrdiff_t index, std::size_t size) {}
			template <std::size_t I, std::size_t ...Is, typename Tuple>
			static void construct_range_storages(mpl::index_sequence<I, Is...>, Tuple & dest, Tuple & src, std::ptrdiff_t index, std::size_t size)
			{
				get<I>(dest).construct_range(index, std::make_move_iterator(get<I>(src).data()), std::make_move_iterator(get<I>(src).data() + size));
				construct_range_storages(mpl::index_sequence<Is...>{}, dest, src, index, size);
			}

			template <typename Tuple>
			static void deallocate_storages(mpl::index_sequence<>, Tuple & storages, std::size_t capacity) {}
			template <std::size_t I, std::size_t ...Is, typename Tuple>
			static void deallocate_storages(mpl::index_sequence<I, Is...>, Tuple & storages, std::size_t capacity)
			{
				get<I>(storages).deallocate(capacity);
				deallocate_storages(mpl::index_sequence<Is...>{}, storages, capacity);
			}

			template <typename Tuple>
			static void memcpy_range_storages(mpl::index_sequence<>, Tuple & dest, Tuple & src, std::ptrdiff_t index, std::size_t size) {}
			template <std::size_t I, std::size_t ...Is, typename Tuple>
			static void memcpy_range_storages(mpl::index_sequence<I, Is...>, Tuple & dest, Tuple & src, std::ptrdiff_t index, std::size_t size)
			{
				get<I>(dest).memcpy_range(index, std::make_move_iterator(get<I>(src).data()), std::make_move_iterator(get<I>(src).data() + size));
				memcpy_range_storages(mpl::index_sequence<Is...>{}, dest, src, index, size);
			}
		};

		template <typename StorageTraits, typename Types, bool = StorageTraits::static_capacity::value>
		struct array_alloc_static_capacity;
		template <typename StorageTraits, typename ...Ts>
		struct array_alloc_static_capacity<StorageTraits, mpl::type_list<Ts...>, true /*static_capacity*/>
			: array_alloc_helper
		{
			template <typename T>
			using storage_type = typename StorageTraits::template storage_type<T>;

			tuple<storage_type<Ts>...> storages_;

			array_alloc_static_capacity() = default;
			array_alloc_static_capacity(std::size_t capacity)
			{
				assert(capacity == StorageTraits::capacity_value);
			}

			constexpr std::size_t capacity() const { return StorageTraits::capacity_value; }

			tuple<storage_type<Ts>...> & storages() { return storages_; }
			const tuple<storage_type<Ts>...> & storages() const { return storages_; }

			void set_capacity(std::size_t capacity)
			{
				assert(capacity == StorageTraits::capacity_value);
			}

			bool try_grow2(std::size_t old_size) { return false; }
		};
		template <typename StorageTraits, typename ...Ts>
		struct array_alloc_static_capacity<StorageTraits, mpl::type_list<Ts...>, false /*static_capacity*/>
			: array_alloc_helper
		{
			template <typename T>
			using storage_type = typename StorageTraits::template storage_type<T>;

			struct capacity_type
			{
				std::size_t capacity_ = 0;

				capacity_type() = default;
				capacity_type(const capacity_type & other) = default;
				capacity_type(capacity_type && other)
					: capacity_(std::exchange(other.capacity_, 0))
				{}
				capacity_type(std::size_t capacity)
					: capacity_(capacity)
				{}
				capacity_type & operator = (const capacity_type & other) = default;
				capacity_type & operator = (capacity_type && other)
				{
					capacity_ = std::exchange(other.capacity_, 0);

					return *this;
				}
			};

			tuple<storage_type<Ts>...> storages_;
			capacity_type capacity_;

			array_alloc_static_capacity() = default;
			array_alloc_static_capacity(std::size_t capacity)
				: capacity_(capacity)
			{}

			std::size_t capacity() const { return capacity_.capacity_; }

			tuple<storage_type<Ts>...> & storages() { return storages_; }
			const tuple<storage_type<Ts>...> & storages() const { return storages_; }

			void set_capacity(std::size_t capacity) { capacity_.capacity_ = capacity; }

			bool try_grow2(std::size_t old_size)
			{
				assert(old_size == capacity());

				const std::size_t new_capacity = StorageTraits::grow(capacity_.capacity_);
				if (old_size >= new_capacity)
					return false;

				tuple<storage_type<Ts>...> new_storages;
				allocate_storages(mpl::make_index_sequence<sizeof...(Ts)>{}, new_storages, new_capacity);
				if (capacity_.capacity_ > 0)
				{
					construct_range_storages(mpl::make_index_sequence<sizeof...(Ts)>{}, new_storages, storages_, 0, capacity_.capacity_);
					deallocate_storages(mpl::make_index_sequence<sizeof...(Ts)>{}, storages_, capacity_.capacity_);
				}
				storages_ = std::move(new_storages);
				capacity_.capacity_ = new_capacity;

				return true;
			}
		};

		template <typename StorageTraits, typename Types, bool = array_alloc_is_trivially_destructible<StorageTraits>::value>
		struct array_alloc_trivially_destructible
			: array_alloc_static_capacity<StorageTraits, Types>
		{};
		template <typename StorageTraits, typename Types>
		struct array_alloc_trivially_destructible<StorageTraits, Types, false>
			: array_alloc_static_capacity<StorageTraits, Types>
		{
			using base_type = array_alloc_static_capacity<StorageTraits, Types>;

			using base_type::base_type;

			~array_alloc_trivially_destructible()
			{
				if (this->capacity() > 0)
				{
					this->deallocate_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, this->capacity());
				}
			}
			array_alloc_trivially_destructible() = default;
			array_alloc_trivially_destructible(const array_alloc_trivially_destructible &) = default;
			array_alloc_trivially_destructible(array_alloc_trivially_destructible &&) = default;
			array_alloc_trivially_destructible & operator = (const array_alloc_trivially_destructible &) = default;
			array_alloc_trivially_destructible & operator = (array_alloc_trivially_destructible &&) = default;
		};

		template <typename StorageTraits, typename Types, bool = array_alloc_is_trivially_copy_constructible<StorageTraits>::value>
		struct array_alloc_trivially_copy_constructible
			: array_alloc_trivially_destructible<StorageTraits, Types>
		{};
		template <typename StorageTraits, typename Types>
		struct array_alloc_trivially_copy_constructible<StorageTraits, Types, false>
			: array_alloc_trivially_destructible<StorageTraits, Types>
		{
			using base_type = array_alloc_trivially_destructible<StorageTraits, Types>;

			using base_type::base_type;

			array_alloc_trivially_copy_constructible() = default;
			array_alloc_trivially_copy_constructible(const array_alloc_trivially_copy_constructible & other)
				: base_type(other.capacity())
			{
				this->allocate_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, other.capacity());
				this->memcpy_range_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, other.storages_, 0, other.capacity());
			}
			array_alloc_trivially_copy_constructible(array_alloc_trivially_copy_constructible &&) = default;
			array_alloc_trivially_copy_constructible & operator = (const array_alloc_trivially_copy_constructible &) = default;
			array_alloc_trivially_copy_constructible & operator = (array_alloc_trivially_copy_constructible &&) = default;
		};

		template <typename StorageTraits, typename Types, bool = array_alloc_is_trivially_copy_assignable<StorageTraits>::value>
		struct array_alloc_trivially_copy_assignable
			: array_alloc_trivially_copy_constructible<StorageTraits, Types>
		{};
		template <typename StorageTraits, typename Types>
		struct array_alloc_trivially_copy_assignable<StorageTraits, Types, false>
			: array_alloc_trivially_copy_constructible<StorageTraits, Types>
		{
			using base_type = array_alloc_trivially_copy_constructible<StorageTraits, Types>;

			using base_type::base_type;

			array_alloc_trivially_copy_assignable() = default;
			array_alloc_trivially_copy_assignable(const array_alloc_trivially_copy_assignable &) = default;
			array_alloc_trivially_copy_assignable(array_alloc_trivially_copy_assignable &&) = default;
			array_alloc_trivially_copy_assignable & operator = (const array_alloc_trivially_copy_assignable & other)
			{
				if (this->capacity() < other.capacity())
				{
					if (this->capacity() > 0)
					{
						this->deallocate_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, this->capacity());
					}
					this->allocate_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, other.capacity());
					this->set_capacity(other.capacity());
				}
				this->memcpy_range_storages(mpl::make_index_sequence<Types::size>{}, this->storages_, other.storages_, 0, other.capacity());

				return *this;
			}
			array_alloc_trivially_copy_assignable & operator = (array_alloc_trivially_copy_assignable &&) = default;
		};
	}

	template <typename StorageTraits, typename ...Ts>
	class array_alloc
		: enable_copy_constructor<detail::array_alloc_is_copy_constructible<StorageTraits, Ts...>::value>
		, enable_copy_assignment<detail::array_alloc_is_copy_assignable<StorageTraits, Ts...>::value>
	{
	public:
		using storage_traits = StorageTraits;

	private:
		detail::array_alloc_trivially_copy_assignable<StorageTraits, mpl::type_list<Ts...>> data_;

	public:
		template <typename T>
		decltype(auto) get() { return utility::get<T>(data_.storages_); }
		template <typename T>
		decltype(auto) get() const { return utility::get<T>(data_.storages_); }
		template <std::size_t I>
		decltype(auto) get() { return utility::get<I>(data_.storages_); }
		template <std::size_t I>
		decltype(auto) get() const { return utility::get<I>(data_.storages_); }

		constexpr std::size_t capacity() const { return data_.capacity(); }

		bool try_grow(std::size_t old_size)
		{
			if (old_size < data_.capacity())
				return true;

			return data_.try_grow2(old_size);
		}
	};
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
