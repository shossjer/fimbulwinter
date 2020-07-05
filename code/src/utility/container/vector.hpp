#pragma once

#include "utility/container/container.hpp"

#include <cassert>

namespace utility
{
	struct no_reallocate_t
	{
		explicit no_reallocate_t() = default;
	};
	constexpr no_reallocate_t no_reallocate = no_reallocate_t{};

	struct no_failure_t
	{
		explicit no_failure_t() = default;
	};
	constexpr no_failure_t no_failure = no_failure_t{};

	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class vector_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = std::size_t;

		protected:
			typename storage_traits::unpacked storage_;
			typename Storage::iterator end_;

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

			std::size_t size() const { return storage_.index_of(end_); }

		protected:
			void set_cap(typename Storage::position /*cap*/)
			{
			}

			void set_end(typename Storage::iterator end)
			{
				end_ = end;
			}
		};

		template <typename Storage>
		class vector_data_impl<Storage, false /*static capacity*/>
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = std::size_t;

		protected:
			typename storage_traits::unpacked storage_;
			typename Storage::position cap_;
			typename Storage::iterator end_;

		public:
			std::size_t capacity() const { return storage_.index_of(cap_); }

			std::size_t size() const { return storage_.index_of(end_); }

		protected:
			void set_cap(typename Storage::position cap)
			{
				cap_ = cap;
			}

			void set_end(typename Storage::iterator end)
			{
				end_ = end;
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
		auto & storage() { return this->storage_; }
		const auto & storage() const { return this->storage_; }

		auto begin_storage() { return this->storage_.begin(); }
		auto begin_storage() const { return this->storage_.begin(); }

		typename Storage::iterator end_storage() { return this->end_; }
		typename Storage::const_iterator end_storage() const { return this->end_; }

		void copy(const this_type & other)
		{
			this->set_end(this->storage_.construct_range(begin_storage(), other.storage_.data(other.begin_storage()), other.storage_.data(other.end_storage())));
		}

		void move(this_type && other)
		{
			this->set_end(this->storage_.construct_range(begin_storage(), std::make_move_iterator(other.storage_.data(other.begin_storage())), std::make_move_iterator(other.storage_.data(other.end_storage()))));
		}

	protected:
		vector_data() = default;

		explicit vector_data(std::size_t min_capacity)
		{
			if (allocate(ReservationStrategy{}(min_capacity)))
			{
				this->set_end(begin_storage());
			}
		}

		void release()
		{
			this->set_cap(this->storage_.place(0));
			this->set_end(begin_storage());
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
	class basic_vector
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
		constexpr std::size_t capacity() const { return base_type::capacity(); }
		std::size_t size() const { return base_type::size(); }

		auto begin() { return this->storage_.data(this->begin_storage()); }
		auto begin() const { return this->storage_.data(this->begin_storage()); }

		auto end() { return this->storage_.data(this->end_storage()); }
		auto end() const { return this->storage_.data(this->end_storage()); }

		auto data() { return this->storage_.data(this->begin_storage()); }
		auto data() const { return this->storage_.data(this->begin_storage()); }

		reference operator [] (ext::index index) { return data()[index]; }
		const_reference operator [] (ext::index index) const { return data()[index]; }

		void clear()
		{
			this->storage_.destruct_range(this->begin_storage(), this->end_storage());
			this->set_end(this->begin_storage());
		}

		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		template <typename ...Ps>
		bool try_emplace_back(utility::no_failure_t, Ps && ...ps)
		{
			auto end = this->end_storage();
			this->storage_.construct_at_(end, std::forward<Ps>(ps)...);
			this->set_end(++end);

			return true;
		}

		template <typename ...Ps>
		bool try_emplace_back(utility::no_reallocate_t, Ps && ...ps)
		{
			if (this->size() < this->capacity())
				return try_emplace_back(utility::no_failure, std::forward<Ps>(ps)...);

			return false;
		}

		template <typename ...Ps>
		bool try_emplace_back(Ps && ...ps)
		{
			if (try_emplace_back(utility::no_reallocate, std::forward<Ps>(ps)...))
				return true;

			if (this->try_reserve(this->size() + 1))
				return try_emplace_back(utility::no_failure, std::forward<Ps>(ps)...);

			return false;
		}

		bool try_erase(ext::index index)
		{
			if (!/*debug_assert*/(static_cast<std::size_t>(index) < this->size()))
				return false;

			const auto last = --this->end_storage();

			using utility::iter_move;
			data()[index] = iter_move(this->storage_.data(last));
			this->storage_.destruct_at(last);
			this->set_end(last);

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
