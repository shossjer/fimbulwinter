
#ifndef UTILITY_ARRAY_ALLOC_HPP
#define UTILITY_ARRAY_ALLOC_HPP

#include "utility/container/container.hpp"
#include "utility/storage.hpp"
#include "utility/utility.hpp"

#include <cassert>

namespace utility
{
	struct initialize_empty
	{
		template <typename Data>
		void operator () (Data & data)
		{
			data.set_capacity(0);
			data.set_size(0);
			data.initialize_empty();
		}
	};

	struct initialize_zero
	{
		template <typename Data>
		void operator () (Data & data)
		{
			const auto capacity = Data::storage_traits::capacity_for(1);
			if (data.storage_.allocate(capacity))
			{
				data.set_capacity(capacity);
				data.set_size(capacity);
				data.initialize_fill(utility::zero_initialize);
			}
			else
			{
				data.set_capacity(0);
				data.set_size(0);
				data.initialize_empty();
			}
		}
	};

	struct reallocate_move
	{
		template <typename Data>
		bool operator () (Data & new_data, Data & old_data)
		{
			new_data.move(std::move(old_data));
			return true;
		}
	};

	template <std::size_t N>
	using size_type_for =
		mpl::conditional_t<(N < 0x100), std::uint8_t,
		mpl::conditional_t<(N < 0x10000), std::uint16_t,
		mpl::conditional_t<(N < 0x100000000), std::uint32_t, std::uint64_t>>>;

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

		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class vector_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = utility::size_type_for<storage_traits::capacity_value>;

		protected:
			size_type size_;
			Storage storage_;

		protected:
			vector_data_impl()
				: size_{}
			{}
			explicit vector_data_impl(utility::null_place_t) {}

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

		protected:
			vector_data_impl()
				: size_{}
				, capacity_{}
			{}
			explicit vector_data_impl(utility::null_place_t) {}

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

			new_data.set_capacity(0);
			new_data.set_size(0); //
			return true;
		}
	};

	template <typename Storage, typename InitializationStrategy, typename ReallocationStrategy>
	struct vector_data
		: detail::vector_data_impl<Storage>
	{
		using this_type = vector_data<Storage, InitializationStrategy, ReallocationStrategy>;
		using base_type = detail::vector_data_impl<Storage>;

		friend InitializationStrategy;
		friend ReallocationStrategy;

	public:
		using storage_type = Storage;

		using storage_traits = utility::storage_traits<Storage>;

		// todo remove these
		using is_trivially_destructible = storage_is_trivially_destructible<Storage>;
		using is_trivially_default_constructible = mpl::false_type;

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
				this->set_size(0);
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
			new_data.set_size(0);
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

	template <typename Data>
	class basic_vector
		: public basic_container<Data>
	{
		using base_type = basic_container<Data>;

		using typename base_type::storage_type;
		using typename base_type::storage_traits;

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
		pointer begin() { return this->storage().data(); }
		const_pointer begin() const { return this->storage().data(); }
		pointer end() { return this->storage().data() + this->size(); }
		const_pointer end() const { return this->storage().data() + this->size(); }

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

			const auto new_capacity = storage_traits::capacity_for(min_capacity);
			if (new_capacity < min_capacity)
				return false;

			return this->try_reallocate(new_capacity);
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
			if (!debug_assert(index < this->size()))
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
	          typename InitializationStrategy = initialize_empty,
	          typename ReallocationStrategy = reallocate_move>
	using array = basic_array<array_data<Storage, InitializationStrategy, ReallocationStrategy>>;
	template <typename Storage,
	          typename InitializationStrategy = initialize_empty,
	          typename ReallocationStrategy = reallocate_move>
	using vector = basic_vector<vector_data<Storage, initialize_empty, ReallocationStrategy>>;

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_array = array<dynamic_storage<Allocator, Ts...>>;
	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_vector = vector<dynamic_storage<Allocator, Ts...>>;

	template <typename ...Ts>
	using heap_array = array<heap_storage<Ts...>>;
	template <typename ...Ts>
	using heap_vector = vector<heap_storage<Ts...>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_array = array<static_storage<Capacity, Ts...>>;
	template <std::size_t Capacity, typename ...Ts>
	using static_vector = vector<static_storage<Capacity, Ts...>>;
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
