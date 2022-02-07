#pragma once

#include "utility/annotate.hpp"
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

	struct stable_t
	{
		explicit stable_t() = default;
	};
	constexpr stable_t stable = stable_t{};

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
			auto position_end() const { return utility::select_first<storage_traits::size>(end_); }
			auto position_cap() const { return storage_.place(capacity()); }

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
			auto position_end() const { return utility::select_first<storage_traits::size>(end_); }
			auto position_cap() const { return cap_; }

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

		using is_trivially_copy_constructible = mpl::false_type;
		using is_trivially_copy_assignable = mpl::false_type;
		using is_trivially_move_constructible = mpl::false_type;
		using is_trivially_move_assignable = mpl::false_type;

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

		void init(const this_type & other)
		{
			if (!StorageTraits::moves_allocation::value)
			{
				this->set_cap(this->storage_.place(other.capacity()));
				this->set_end(begin_storage() + other.size());
			}
		}

		void release()
		{
			if (StorageTraits::moves_allocation::value)
			{
				this->set_cap(this->storage_.place(0));
				this->set_end(begin_storage());
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
			return ReservationStrategy{}(other.size());
		}

		constexpr bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.size());
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
		using iterator = pointer;
		using const_iterator = const_pointer;

	public:
		annotate_nodiscard
		constexpr std::size_t capacity() const { return base_type::capacity(); }
		annotate_nodiscard
		std::size_t size() const { return base_type::size(); }

		annotate_nodiscard
		iterator begin() { return this->storage_.data(this->begin_storage()); }
		annotate_nodiscard
		const_iterator begin() const { return this->storage_.data(this->begin_storage()); }

		annotate_nodiscard
		iterator end() { return this->storage_.data(this->end_storage()); }
		annotate_nodiscard
		const_iterator end() const { return this->storage_.data(this->end_storage()); }

		annotate_nodiscard
		pointer data() { return this->storage_.data(this->begin_storage()); }
		annotate_nodiscard
		const_pointer data() const { return this->storage_.data(this->begin_storage()); }

		annotate_nodiscard
		reference operator [] (ext::index index) { return data()[index]; }
		annotate_nodiscard
		const_reference operator [] (ext::index index) const { return data()[index]; }

		void clear()
		{
			this->storage_.destruct_range(this->begin_storage(), this->end_storage());
			this->set_end(this->begin_storage());
		}

		annotate_nodiscard
		bool try_reserve(std::size_t min_capacity)
		{
			if (fiw_likely(min_capacity <= this->capacity()))
				return true;

			return this->try_reallocate(min_capacity);
		}

		template <typename ...Ps>
		annotate_nodiscard
		bool resize(std::size_t size, Ps && ...ps)
		{
			if (size < this->size())
			{
				const auto it = this->begin_storage() + size;

				this->storage_.destruct_range(it, this->end_storage());
				this->set_end(it);
			}
			else
			{
				if (size > this->capacity())
				{
					if (!this->try_reallocate(size))
						return false;
				}

				this->set_end(this->storage_.construct_fill(this->end_storage(), size - this->size(), static_cast<Ps &&>(ps)...));
			}

			return true;
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
		annotate_nodiscard
		bool try_emplace_back(utility::no_reallocate_t, Ps && ...ps)
		{
			if (fiw_likely(this->position_end() != this->position_cap()))
				return try_emplace_back(utility::no_failure, std::forward<Ps>(ps)...);

			return false;
		}

		template <typename ...Ps>
		annotate_nodiscard
		bool try_emplace_back(Ps && ...ps)
		{
			if (fiw_likely(try_emplace_back(utility::no_reallocate, std::forward<Ps>(ps)...)))
				return true;

			if (this->try_reserve(this->size() + 1))
				return try_emplace_back(utility::no_failure, std::forward<Ps>(ps)...);

			return false;
		}

		annotate_nodiscard
		bool push_back(const_reference p) { return try_emplace_back(p); }
		annotate_nodiscard
		bool push_back(rvalue_reference p) { return try_emplace_back(std::move(p)); }

		template <typename BeginIt, typename EndIt>
		bool push_back(utility::no_failure_t, BeginIt begin, EndIt end)
		{
			for (; begin != end; ++begin)
			{
				try_emplace_back(utility::no_failure, *begin);
			}
			return true;
		}

		template <typename P>
		annotate_nodiscard
		bool insert(iterator it, P && p)
		{
			if (it != end())
			{
				using utility::iter_move;
				if (try_emplace_back(iter_move(it)))
				{
					*it = std::move(p);

					return true;
				}
				return false;
			}
			else
			{
				return try_emplace_back(std::move(p));
			}
		}

		iterator erase(const_iterator it)
		{
			iterator nonit = undo_const(it);
			if (!/*debug_assert*/(begin() <= it && it < end()))
				return nonit;

			auto last = this->end_storage();
			--last;

			using utility::iter_move;
			*nonit = iter_move(this->storage_.data(last));
			this->storage_.destruct_at(last);
			this->set_end(last);

			return nonit;
		}

		iterator erase(const_iterator from, const_iterator to)
		{
			iterator nonfrom = undo_const(from);
			if (!/*debug_assert*/(begin() <= from && from <= to && to <= end()))
				return nonfrom;

			auto last = this->end_storage();

			// idea split loop into two
			for (iterator it = nonfrom; it != to; ++it)
			{
				--last;

				using utility::iter_move;
				*it = iter_move(this->storage_.data(last));
				this->storage_.destruct_at(last);
			}
			this->set_end(last);

			return nonfrom;
		}

		iterator erase(utility::stable_t, const_iterator it)
		{
			iterator nonit = undo_const(it);
			if (!/*debug_assert*/(begin() <= it && it < end()))
				return nonit;

			iterator write = nonit;
			for (iterator read = nonit + 1; read != end(); ++write, ++read)
			{
				using utility::iter_move;
				*write = iter_move(read);
			}
			this->storage_.destruct_at(this->storage_.iter(write));
			this->set_end(this->storage_.iter(write));

			return nonit;
		}

		iterator erase(utility::stable_t, const_iterator from, const_iterator to)
		{
			iterator nonfrom = undo_const(from);
			if (!/*debug_assert*/(begin() <= from && from <= to && to <= end()))
				return nonfrom;

			iterator write = nonfrom;
			for (iterator read = undo_const(to); read != end(); ++write, ++read)
			{
				using utility::iter_move;
				*write = iter_move(read);
			}
			this->storage_.destruct_range(this->storage_.iter(write), this->end_storage());
			this->set_end(this->storage_.iter(write));

			return nonfrom;
		}
	};

	template <typename Data>
	constexpr std::size_t capacity(const basic_vector<Data> & x) { return x.capacity(); }
	template <typename Data>
	std::size_t size(const basic_vector<Data> & x) { return x.size(); }

	template <typename Data>
	typename basic_vector<Data>::iterator begin(basic_vector<Data> & x) { return x.begin(); }
	template <typename Data>
	typename basic_vector<Data>::const_iterator begin(const basic_vector<Data> & x) { return x.begin(); }

	template <typename Data>
	typename basic_vector<Data>::iterator end(basic_vector<Data> & x) { return x.end(); }
	template <typename Data>
	typename basic_vector<Data>::const_iterator end(const basic_vector<Data> & x) { return x.end(); }

	template <typename Data>
	typename basic_vector<Data>::pointer data(basic_vector<Data> & x) { return x.data(); }
	template <typename Data>
	typename basic_vector<Data>::const_pointer data(const basic_vector<Data> & x) { return x.data(); }

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

namespace ext
{
	template <typename Data>
	decltype(auto) back(utility::basic_vector<Data> & vector)
	{
		auto it = vector.end();
		return *--it;
	}

	template <typename Data>
	decltype(auto) back(const utility::basic_vector<Data> & vector)
	{
		auto it = vector.end();
		return *--it;
	}

	template <typename Data>
	decltype(auto) back(utility::basic_vector<Data> && vector)
	{
		auto it = vector.end();
		using utility::iter_move;
		return iter_move(--it);
	}

	template <typename Data>
	decltype(auto) back(const utility::basic_vector<Data> && vector)
	{
		auto it = vector.end();
		using utility::iter_move;
		return iter_move(--it);
	}

	template <typename Data>
	decltype(auto) empty(const utility::basic_vector<Data> & vector) { return vector.begin() == vector.end(); }

	template <typename Data>
	decltype(auto) front(utility::basic_vector<Data> & vector)
	{
		return *vector.begin();
	}

	template <typename Data>
	decltype(auto) front(const utility::basic_vector<Data> & vector)
	{
		return *vector.begin();
	}

	template <typename Data>
	decltype(auto) front(utility::basic_vector<Data> && vector)
	{
		using utility::iter_move;
		return iter_move(vector.begin());
	}

	template <typename Data>
	decltype(auto) front(const utility::basic_vector<Data> && vector)
	{
		using utility::iter_move;
		return iter_move(vector.begin());
	}

	template <typename Data>
	void pop_back(utility::basic_vector<Data> & vector)
	{
		auto it = vector.end();
		vector.erase(--it);
	}

	template <typename Data>
	void pop_back(utility::basic_vector<Data> && vector)
	{
		auto it = vector.end();
		vector.erase(--it);
	}
}
