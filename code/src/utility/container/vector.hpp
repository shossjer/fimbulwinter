#pragma once

#include "utility/container/container.hpp"

#include <cassert>

namespace utility
{
	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class vector_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = utility::size_type_for<storage_traits::capacity_value>;

		protected:
			size_type size_;
			Storage storage_;

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

			std::size_t size() const { return size_; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				assert(capacity == 0 || capacity == storage_traits::capacity_value);
				static_cast<void>(capacity);
			}

			void set_size(std::size_t size)
			{
				assert(size <= size_type(-1) && size <= storage_traits::capacity_value);

				size_ = static_cast<size_type>(size);
			}
		};

		template <typename Storage>
		class vector_data_impl<Storage, false /*static capacity*/>
		{
		public:
			using size_type = std::size_t;

		protected:
			size_type size_;
			size_type capacity_;
			Storage storage_;

		public:
			std::size_t capacity() const { return capacity_; }

			std::size_t size() const { return size_; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				capacity_ = capacity;
			}

			void set_size(std::size_t size)
			{
				assert(size <= capacity_);

				size_ = size;
			}
		};
	}

	template <typename Storage, typename ReservationStrategy, typename RelocationStrategy>
	class vector_data
		: public detail::vector_data_impl<Storage>
	{
		using this_type = vector_data<Storage, ReservationStrategy, RelocationStrategy>;
		using base_type = detail::vector_data_impl<Storage>;

		using StorageTraits = utility::storage_traits<Storage>;

	public:
		using storage_type = Storage;

		using is_trivially_destructible =
			mpl::conjunction<typename Storage::storing_trivially_destructible,
			                 typename StorageTraits::trivial_deallocate>;

		using is_trivially_default_constructible = mpl::false_type;

	public:
		auto storage() { return this->storage_.sections(this->capacity()); }
		auto storage() const { return this->storage_.sections(this->capacity()); }

		void copy(const this_type & other)
		{
			this->set_size(other.size());

			storage().construct_range(0, other.storage().data() + 0, other.storage().data() + other.size());
		}

		void move(this_type && other)
		{
			this->set_size(other.size());

			storage().construct_range(0, std::make_move_iterator(other.storage().data() + 0), std::make_move_iterator(other.storage().data() + other.size()));
		}

	protected:
		vector_data() = default;

		explicit vector_data(std::size_t min_capacity)
		{
			if (allocate(ReservationStrategy{}(min_capacity)))
			{
				this->set_size(0);
			}
		}

		void release()
		{
			this->set_capacity(0);
			this->set_size(0);
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
			return ReservationStrategy{}(other.size());
		}

		constexpr bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.size());
		}

		void purge()
		{
			if (this->storage_.good())
			{
				storage().destruct_range(0, this->size());
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
	class basic_vector
		: public basic_container<Data>
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
		auto begin() { return this->storage().data() + 0; }
		auto begin() const { return this->storage().data() + 0; }
		auto end() { return this->storage().data() + this->size(); }
		auto end() const { return this->storage().data() + this->size(); }

		pointer data() { return this->storage().data(); }
		const_pointer data() const { return this->storage().data(); }

		reference operator [] (ext::index index) { return this->storage().data()[index]; }
		const_reference operator [] (ext::index index) const { return this->storage().data()[index]; }

		void clear()
		{
			this->storage().destruct_range(0, this->size());
			this->set_size(0);
		}

		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		template <typename ...Ps>
		bool try_emplace_back(Ps && ...ps)
		{
			if (!this->try_reserve(this->size() + 1))
				return false;

			this->storage().construct_at(this->size(), std::forward<Ps>(ps)...);
			this->set_size(this->size() + 1);

			return true;
		}

		bool try_erase(ext::index index)
		{
			if (!/*debug_assert*/(static_cast<std::size_t>(index) < this->size()))
				return false;

			const auto last = this->size() - 1;
			auto storage = this->storage();

			storage[index] = std::move(storage[last]);
			storage.destruct_at(last);
			this->set_size(last);

			return true;
		}
	};

	template <typename Storage,
	          template <typename> class ReservationStrategy = utility::reserve_power_of_two,
	          typename RelocationStrategy = utility::relocate_move>
	using vector = basic_vector<vector_data<Storage,
	                                        ReservationStrategy<Storage>,
	                                        RelocationStrategy>>;

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_vector = vector<dynamic_storage<Allocator, Ts...>>;

	template <typename ...Ts>
	using heap_vector = vector<heap_storage<Ts...>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_vector = vector<static_storage<Capacity, Ts...>>;
}
