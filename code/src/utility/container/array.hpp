#pragma once

#include "utility/annotate.hpp"
#include "utility/container/container.hpp"

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

			using is_trivially_copy_constructible = std::is_trivially_copy_constructible<Storage>;
			using is_trivially_copy_assignable = std::is_trivially_copy_assignable<Storage>;
			using is_trivially_move_constructible = std::is_trivially_move_constructible<Storage>;
			using is_trivially_move_assignable = std::is_trivially_move_assignable<Storage>;

		protected:
			typename storage_traits::unpacked storage_;

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

		protected:
			void set_cap(typename Storage::position /*cap*/)
			{
			}
		};

		template <typename Storage>
		class array_data_impl<Storage, false /*static capacity*/>
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = std::size_t;

			using is_trivially_copy_constructible = mpl::false_type;
			using is_trivially_copy_assignable = mpl::false_type;
			using is_trivially_move_constructible = mpl::false_type;
			using is_trivially_move_assignable = mpl::false_type;

		protected:
			typename storage_traits::unpacked storage_;
			typename Storage::position cap_;

		public:
			std::size_t capacity() const { return storage_.index_of(cap_); }

		protected:
			void set_cap(typename Storage::position cap)
			{
				cap_ = cap;
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
		auto & storage() { return this->storage_; }
		const auto & storage() const { return this->storage_; }

		auto begin_storage() { return this->storage_.begin(); }
		auto begin_storage() const { return this->storage_.begin(); }

		auto end_storage() { return this->storage_.begin() + this->capacity(); }
		auto end_storage() const { return this->storage_.begin() + this->capacity(); }

		void copy(const this_type & other)
		{
			const auto end = this->storage_.construct_range(begin_storage(), other.storage_.data(other.begin_storage()), other.storage_.data(other.end_storage()));

			InitializationStrategy{}([this, &other, &end](auto && ...ps){ this->storage_.construct_fill(end, this->capacity() - other.capacity(), std::forward<decltype(ps)>(ps)...); });
		}

		void move(this_type && other)
		{
			const auto end = this->storage_.construct_range(begin_storage(), std::make_move_iterator(other.storage_.data(other.begin_storage())), std::make_move_iterator(other.storage_.data(other.end_storage())));

			InitializationStrategy{}([this, &other, &end](auto && ...ps){ this->storage_.construct_fill(end, this->capacity() - other.capacity(), std::forward<decltype(ps)>(ps)...); });
		}

	protected:
		array_data() = default;

		explicit array_data(std::size_t size)
		{
			if (allocate(ReservationStrategy{}(size)))
			{
				InitializationStrategy{}([this](auto && ...ps){ this->storage_.construct_fill(begin_storage(), this->capacity(), std::forward<decltype(ps)>(ps)...); });
			}
		}

		void init(const this_type & other)
		{
			if (!StorageTraits::moves_allocation::value)
			{
				this->set_cap(this->storage_.place(other.capacity()));
			}
		}

		void release()
		{
			if (StorageTraits::moves_allocation::value)
			{
				this->set_cap(this->storage_.place(0));
			}
		}

		bool allocate(std::size_t capacity)
		{
			if (this->storage_.allocate(capacity))
			{
				this->set_cap(this->storage_.place(capacity));
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
			this->storage_.destruct_range(begin_storage(), end_storage());
		}

		void purge()
		{
			if (this->storage_.good())
			{
				this->storage_.destruct_range(begin_storage(), end_storage());
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

		annotate_nodiscard
		constexpr std::size_t capacity() const { return base_type::capacity(); }
		annotate_nodiscard
		constexpr std::size_t size() const { return capacity(); }

		annotate_nodiscard
		pointer begin() { return this->storage_.data(this->begin_storage()); }
		annotate_nodiscard
		const_pointer begin() const { return this->storage_.data(this->begin_storage()); }

		annotate_nodiscard
		pointer end() { return this->storage_.data(this->end_storage()); }
		annotate_nodiscard
		const_pointer end() const { return this->storage_.data(this->end_storage()); }

		annotate_nodiscard
		auto data() { return this->storage_.data(this->begin_storage()); }
		annotate_nodiscard
		auto data() const { return this->storage_.data(this->begin_storage()); }

		decltype(auto) operator [] (ext::index i) { return data()[i]; }
		decltype(auto) operator [] (ext::index i) const { return data()[i]; }

		annotate_nodiscard
		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		// todo this is only correct if ReservationStrategy is exact
		annotate_nodiscard
		bool resize(std::size_t size)
		{
			return this->try_reallocate(size);
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
