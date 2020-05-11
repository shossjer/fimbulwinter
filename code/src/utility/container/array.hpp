#pragma once

#include "utility/container/container.hpp"

#include <cassert>

namespace utility
{
	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class array_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = utility::size_type_for<storage_traits::capacity_value>;

		protected:
			Storage storage_;

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				assert(capacity == 0 || capacity == storage_traits::capacity_value);
				static_cast<void>(capacity);
			}
		};

		template <typename Storage>
		class array_data_impl<Storage, false /*static capacity*/>
		{
		public:
			using size_type = std::size_t;

		protected:
			size_type capacity_;
			Storage storage_;

		public:
			std::size_t capacity() const { return capacity_; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				capacity_ = capacity;
			}
		};
	}

	template <typename Storage, typename InitializationStrategy, typename ReservationStrategy, typename RelocationStrategy>
	class array_data
		: public detail::array_data_impl<Storage>
	{
		using this_type = array_data<Storage, InitializationStrategy, ReservationStrategy, RelocationStrategy>;
		using base_type = detail::array_data_impl<Storage>;

		using StorageTraits = utility::storage_traits<Storage>;

	public:
		using storage_type = Storage;

		using is_trivially_destructible =
			mpl::conjunction<typename Storage::storing_trivially_destructible,
			                 typename StorageTraits::trivial_deallocate>;

		using is_trivially_default_constructible =
			mpl::conjunction<typename StorageTraits::trivial_allocate,
			                 typename StorageTraits::static_capacity,
			                 typename InitializationStrategy::is_trivial>;

	public:
		auto storage() { return this->storage_.sections(this->capacity()); }
		auto storage() const { return this->storage_.sections(this->capacity()); }

		void copy(const this_type & other)
		{
			storage().construct_range(0, other.storage().data() + 0, other.storage().data() + other.capacity());

			InitializationStrategy{}([this, &other](auto && ...ps){ storage().construct_fill(other.capacity(), this->capacity(), std::forward<decltype(ps)>(ps)...); });
		}

		void move(this_type && other)
		{
			storage().construct_range(0, std::make_move_iterator(other.storage().data() + 0), std::make_move_iterator(other.storage().data() + other.capacity()));

			InitializationStrategy{}([this, &other](auto && ...ps){ storage().construct_fill(other.capacity(), this->capacity(), std::forward<decltype(ps)>(ps)...); });
		}

	protected:
		array_data() = default;

		explicit array_data(std::size_t size)
		{
			if (allocate(ReservationStrategy{}(size)))
			{
				InitializationStrategy{}([this](auto && ...ps){ this->storage().construct_fill(0, this->capacity(), std::forward<decltype(ps)>(ps)...); });
			}
		}

		void release()
		{
			this->set_capacity(0);
		}

		bool allocate(std::size_t capacity)
		{
			if (this->storage_.allocate(capacity))
			{
				this->set_capacity(capacity);
				return true;
			}
			else
			{
				release();
				return false;
			}
		}

		constexpr std::size_t capacity_for(const this_type & other) const
		{
			return other.capacity();
		}

		constexpr bool fits(const this_type & other) const
		{
			return this->capacity() == other.capacity();
		}

		void clear()
		{
			storage().destruct_range(0, this->capacity());
		}

		void purge()
		{
			if (this->storage_.good())
			{
				storage().destruct_range(0, this->capacity());
				this->storage_.deallocate(this->capacity());
			}
		}

		constexpr bool try_reallocate(std::size_t min_capacity)
		{
			return try_reallocate_impl(typename StorageTraits::static_capacity{}, min_capacity);
		}

	private:
		constexpr bool try_reallocate_impl(mpl::true_type /*static capacity*/, std::size_t /*min_capacity*/)
		{
			return false;
		}

		bool try_reallocate_impl(mpl::false_type /*static capacity*/, std::size_t min_capacity)
		{
			const auto new_capacity = ReservationStrategy{}(min_capacity);
			if (new_capacity < min_capacity)
				return false;

			this_type new_data;
			if (!new_data.allocate(new_capacity))
				return false;

			if (!RelocationStrategy{}(new_data, *this))
				return false;

			this->purge();

			*this = std::move(new_data);

			return true;
		}
	};

	template <typename Data>
	class basic_array
		: basic_container<Data>
	{
		using base_type = basic_container<Data>;

		using typename base_type::storage_type;

	public:
		using value_type = typename storage_type::value_type;
		using size_type = typename storage_type::size_type;
		using difference_type = typename storage_type::difference_type;
		using reference = typename storage_type::reference;
		using const_reference = typename storage_type::const_reference;
		using rvalue_reference = typename storage_type::rvalue_reference;
		using pointer = typename storage_type::pointer;
		using const_pointer = typename storage_type::const_pointer;

	public:
		basic_array() = default;

		explicit basic_array(std::size_t size)
			: base_type(size)
		{}

		constexpr std::size_t capacity() const { return base_type::capacity(); }
		constexpr std::size_t size() const { return capacity(); }

		pointer data() { return this->storage().data(); }
		const_pointer data() const { return this->storage().data(); }

		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}
	};

	template <typename Storage,
	          template <typename> class InitializationStrategy = utility::initialize_default,
	          template <typename> class ReservationStrategy = utility::reserve_exact,
	          typename RelocationStrategy = utility::relocate_move>
	using array = basic_array<array_data<Storage,
	                                     InitializationStrategy<Storage>,
	                                     ReservationStrategy<Storage>,
	                                     RelocationStrategy>>;

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_array = array<dynamic_storage<Allocator, Ts...>>;

	template <typename ...Ts>
	using heap_array = array<heap_storage<Ts...>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_array = array<static_storage<Capacity, Ts...>>;
}
