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

		protected:
			array_data_impl() = default;
			explicit array_data_impl(utility::null_place_t) {}

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

			constexpr std::size_t size() const { return storage_traits::capacity_value; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				assert(capacity == 0 || capacity == storage_traits::capacity_value);
				static_cast<void>(capacity);
			}

			void set_size(std::size_t size)
			{
				assert(size == 0 || size == this->capacity());
				static_cast<void>(size);
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

		protected:
			array_data_impl()
				: capacity_{}
			{}
			explicit array_data_impl(utility::null_place_t) {}

		public:
			std::size_t capacity() const { return capacity_; }

			std::size_t size() const { return capacity_; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				capacity_ = capacity;
			}

			void set_size(std::size_t size)
			{
				assert(size == 0 || size == this->capacity());
				static_cast<void>(size);
			}
		};
	}

	template <typename Storage, typename InitializationStrategy, typename ReallocationStrategy>
	class array_data
		: public detail::array_data_impl<Storage>
	{
		using this_type = array_data<Storage, InitializationStrategy, ReallocationStrategy>;
		using base_type = detail::array_data_impl<Storage>;

		friend InitializationStrategy;
		friend ReallocationStrategy;

	public:
		using storage_type = Storage;

		using storage_traits = utility::storage_traits<Storage>;

		// todo remove these
		using is_trivially_destructible = utility::storage_is_trivially_destructible<Storage>;
		using is_trivially_default_constructible =
			mpl::conjunction<typename storage_traits::static_capacity,
			                 utility::storage_is_trivially_default_constructible<Storage>>;

	public: // todo weird to allow write access
		template <std::size_t I>
		auto section(mpl::index_constant<I>) { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<I>{}); }
		template <std::size_t I>
		auto section(mpl::index_constant<I>) const { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<I>{}); }

	protected:
		auto storage() { return this->storage_.sections(this->capacity()); }
		auto storage() const { return this->storage_.sections(this->capacity()); }

		using base_type::base_type;

		bool allocate(const this_type & other)
		{
			const auto capacity = storage_traits::capacity_for(other.size());
			if (this->storage_.allocate(capacity))
			{
				this->set_capacity(capacity);
				return true;
			}
			else
			{
				this->set_capacity(0);
				this->set_size(0); //
				return false;
			}
		}

		constexpr bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.size());
		}

		void clear()
		{
			if (0 < this->capacity())
			{
				storage().destruct_range(0, this->size());
			}
		}

		void purge()
		{
			if (0 < this->capacity())
			{
				storage().destruct_range(0, this->size());
				this->storage_.deallocate(this->capacity());
			}
		}

		void copy(const this_type & other)
		{
			this->set_size(other.size());

			storage().construct_range(0, other.storage().data(), other.storage().data() + other.size());
		}

		void move(this_type && other)
		{
			this->set_size(other.size());

			storage().construct_range(0, std::make_move_iterator(other.storage().data()), std::make_move_iterator(other.storage().data() + other.size()));
		}

		void initialize()
		{
			InitializationStrategy{}(*this);
		}

		constexpr bool try_reallocate(std::size_t new_capacity)
		{
			return try_reallocate_impl(typename storage_traits::static_capacity{}, new_capacity);
		}

	private:
		void initialize_empty()
		{
		}

		template <typename ...Ps>
		void initialize_fill(Ps && ...ps)
		{
			storage().construct_fill(0, this->capacity(), std::forward<Ps>(ps)...);
		}

		constexpr bool try_reallocate_impl(mpl::true_type /*static capacity*/, std::size_t /*new_capacity*/)
		{
			return false;
		}

		bool try_reallocate_impl(mpl::false_type /*static capacity*/, std::size_t new_capacity)
		{
			this_type new_data;
			if (!new_data.storage_.allocate(new_capacity))
				return false;

			new_data.set_capacity(new_capacity);

			if (!ReallocationStrategy{}(new_data, *this))
				return false;

			this->purge();

			*this = std::move(new_data);

			// todo is this even necessary?
			new_data.set_capacity(0);
			new_data.set_size(0); //
			return true;
		}
	};

	template <typename Data>
	class basic_array
		: public basic_container<Data>
	{
		using base_type = basic_container<Data>;

	private:
		using typename Data::storage_traits;

	public:
		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			const auto new_capacity = storage_traits::capacity_for(min_capacity);
			if (new_capacity < min_capacity)
				return false;

			return this->try_reallocate(new_capacity);
		}
	};

	template <typename Storage,
	          typename InitializationStrategy = initialize_empty,
	          typename ReallocationStrategy = reallocate_move>
	using array = basic_array<array_data<Storage, InitializationStrategy, ReallocationStrategy>>;

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_array = array<dynamic_storage<Allocator, Ts...>>;

	template <typename ...Ts>
	using heap_array = array<heap_storage<Ts...>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_array = array<static_storage<Capacity, Ts...>>;
}
