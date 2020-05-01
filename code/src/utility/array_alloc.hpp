
#ifndef UTILITY_ARRAY_ALLOC_HPP
#define UTILITY_ARRAY_ALLOC_HPP

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

		// protected: //
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

		// protected: //
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

		// protected: //
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

		// protected: // todo
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

	template <typename Storage, typename InitializationStrategy>
	class array_data
		: public detail::array_data_impl<Storage>
	{
		using this_type = array_data<Storage, InitializationStrategy>;
		using base_type = detail::array_data_impl<Storage>;

		friend InitializationStrategy;

	public:
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

	// protected: // todo
		auto storage() { return this->storage_.sections(this->capacity()); }
		auto storage() const { return this->storage_.sections(this->capacity()); }

		using base_type::base_type;

		bool allocate_storage(std::size_t capacity)
		{
			return this->storage_.allocate(capacity);
		}

		void deallocate_storage(std::size_t capacity)
		{
			this->storage_.deallocate(capacity);
		}

		void initialize()
		{
			InitializationStrategy{}(*this);
		}

		void copy_construct_range(std::ptrdiff_t index, const this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->storage_.sections(this->capacity()).construct_range(index, other.storage_.sections(other.capacity()).data() + from, other.storage_.sections(other.capacity()).data() + to);
		}

		void move_construct_range(std::ptrdiff_t index, this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->storage_.sections(this->capacity()).construct_range(index, std::make_move_iterator(other.storage_.sections(other.capacity()).data() + from), std::make_move_iterator(other.storage_.sections(other.capacity()).data() + to));
		}

		void destruct_range(std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->storage_.sections(this->capacity()).destruct_range(from, to);
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
	};

	template <typename Storage, typename InitializationStrategy>
	struct vector_data
		: detail::vector_data_impl<Storage>
	{
		using this_type = vector_data<Storage, InitializationStrategy>;
		using base_type = detail::vector_data_impl<Storage>;

		friend InitializationStrategy;

	public:
		using storage_traits = utility::storage_traits<Storage>;

		// todo remove these
		using is_trivially_destructible = storage_is_trivially_destructible<Storage>;
		using is_trivially_default_constructible = mpl::false_type;

	public: // todo weird to allow write access
		template <std::size_t I>
		auto section(mpl::index_constant<I>) { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<I>{}); }
		template <std::size_t I>
		auto section(mpl::index_constant<I>) const { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<I>{}); }

	// protected: // todo
		auto storage() { return this->storage_.sections(this->capacity()); }
		auto storage() const { return this->storage_.sections(this->capacity()); }

		using base_type::base_type;

		bool allocate_storage(std::size_t capacity)
		{
			return this->storage_.allocate(capacity);
		}

		void deallocate_storage(std::size_t capacity)
		{
			this->storage_.deallocate(capacity);
		}

		void initialize()
		{
			InitializationStrategy{}(*this);
		}

		void copy_construct_range(std::ptrdiff_t index, const this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			auto other_sections = other.storage_.sections(other.capacity());
			this->storage_.sections(this->capacity()).construct_range(index, other_sections.data() + from, other_sections.data() + to);
		}

		void move_construct_range(std::ptrdiff_t index, this_type & other, std::ptrdiff_t from, std::ptrdiff_t to)
		{
			auto other_sections = other.storage_.sections(other.capacity());
			this->storage_.sections(this->capacity()).construct_range(index, std::make_move_iterator(other_sections.data() + from), std::make_move_iterator(other_sections.data() + to));
		}

		void destruct_range(std::ptrdiff_t from, std::ptrdiff_t to)
		{
			this->storage_.sections(this->capacity()).destruct_range(from, to);
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
	};

	namespace detail
	{
		template <typename Data, bool = Data::is_trivially_destructible::value>
		struct container_trivially_destructible
			: Data
		{
			using base_type = Data;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_destructible<Data, false /*trivially destructible*/>
			: Data
		{
			using base_type = Data;
			using this_type = container_trivially_destructible<Data, false>;

			using base_type::base_type;

			~container_trivially_destructible()
			{
				if (this->capacity() > 0)
				{
					this->destruct_range(0, this->size());
					this->deallocate_storage(this->capacity());
				}
			}
			container_trivially_destructible() = default;
			container_trivially_destructible(const this_type &) = default;
			container_trivially_destructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = Data::is_trivially_default_constructible::value>
		struct container_trivially_default_constructible
			: container_trivially_destructible<Data>
		{
			using base_type = container_trivially_destructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_default_constructible<Data, false /*trivially default constructible*/>
			: container_trivially_destructible<Data>
		{
			using base_type = container_trivially_destructible<Data>;
			using this_type = container_trivially_default_constructible<Data, false>;

			using base_type::base_type;

			container_trivially_default_constructible()
				: base_type(utility::null_place)
			{
				this->initialize();
			}
			container_trivially_default_constructible(const this_type &) = default;
			container_trivially_default_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = std::is_copy_constructible<Data>::value>
		struct container_trivially_copy_constructible
			: container_trivially_default_constructible<Data>
		{
			using base_type = container_trivially_default_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_copy_constructible<Data, false /*copy constructible*/>
			: container_trivially_default_constructible<Data>
		{
			using base_type = container_trivially_default_constructible<Data>;
			using this_type = container_trivially_copy_constructible<Data, false>;

			using base_type::base_type;

			container_trivially_copy_constructible() = default;
			container_trivially_copy_constructible(const this_type & other)
				: base_type(utility::null_place)
			{
				const auto capacity = Data::storage_traits::capacity_for(other.size());
				if (this->allocate_storage(capacity))
				{
					this->set_capacity(capacity);

					this->set_size(other.size());
					this->copy_construct_range(0, other, 0, other.size());
				}
				else
				{
					this->set_capacity(0);
					this->set_size(0);
				}
			}
			container_trivially_copy_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = std::is_copy_assignable<Data>::value>
		struct container_trivially_copy_assignable
			: container_trivially_copy_constructible<Data>
		{
			using base_type = container_trivially_copy_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_copy_assignable<Data, false /*copy assignable*/>
			: container_trivially_copy_constructible<Data>
		{
			using base_type = container_trivially_copy_constructible<Data>;
			using this_type = container_trivially_copy_assignable<Data, false>;

			using base_type::base_type;

			container_trivially_copy_assignable() = default;
			container_trivially_copy_assignable(const this_type &) = default;
			container_trivially_copy_assignable(this_type &&) = default;
			this_type & operator = (const this_type & other)
			{
				if (this->capacity() < other.size())
				{
					if (this->capacity() > 0)
					{
						this->destruct_range(0, this->size());
						this->deallocate_storage(this->capacity());
					}
					const auto capacity = Data::storage_traits::capacity_for(other.size());
					if (!this->allocate_storage(capacity))
					{
						this->set_capacity(0);
						this->set_size(0);

						return *this;
					}
					this->set_capacity(capacity);
				}
				else if (this->capacity() > 0)
				{
					this->destruct_range(0, this->size());
				}
				this->set_size(other.size());
				this->copy_construct_range(0, other, 0, other.size());

				return *this;
			}
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool move_constructible = std::is_move_constructible<Data>::value, bool moves_allocation = Data::storage_traits::moves_allocation::value>
		struct container_trivially_move_constructible
			: container_trivially_copy_assignable<Data>
		{
			static_assert(move_constructible && moves_allocation, "");

			using base_type = container_trivially_copy_assignable<Data>;
			using this_type = container_trivially_move_constructible<Data, move_constructible, moves_allocation>;

			using base_type::base_type;

			container_trivially_move_constructible() = default;
			container_trivially_move_constructible(const this_type &) = default;
			container_trivially_move_constructible(this_type && other)
				: base_type(std::move(other))
			{
				other.initialize();
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};
		template <typename Data>
		struct container_trivially_move_constructible<Data, true /*move constructible*/, false /*moves allocation*/>
			: container_trivially_copy_assignable<Data>
		{
			using base_type = container_trivially_copy_assignable<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_move_constructible<Data, false /*move constructible*/, false /*moves allocation*/>
			: container_trivially_copy_assignable<Data>
		{
			using base_type = container_trivially_copy_assignable<Data>;
			using this_type = container_trivially_move_constructible<Data, false, false>;

			using base_type::base_type;

			container_trivially_move_constructible() = default;
			container_trivially_move_constructible(const this_type &) = default;
			container_trivially_move_constructible(this_type && other)
				: base_type(utility::null_place)
			{
				const auto capacity = Data::storage_traits::capacity_for(other.size());
				if (this->allocate_storage(capacity))
				{
					this->set_capacity(capacity);

					this->set_size(other.size());
					this->move_construct_range(0, other, 0, other.size());
				}
				else
				{
					this->set_capacity(0);
					this->set_size(0);
				}
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool move_assignable = std::is_move_assignable<Data>::value, bool moves_allocation = Data::storage_traits::moves_allocation::value>
		struct container_trivially_move_assignable
			: container_trivially_move_constructible<Data>
		{
			static_assert(move_assignable && moves_allocation, "");

			using base_type = container_trivially_move_constructible<Data>;
			using this_type = container_trivially_move_assignable<Data, move_assignable, moves_allocation>;

			using base_type::base_type;

			container_trivially_move_assignable() = default;
			container_trivially_move_assignable(const this_type &) = default;
			container_trivially_move_assignable(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type && other)
			{
				if (this->capacity() > 0)
				{
					this->destruct_range(0, this->size());
					this->deallocate_storage(this->capacity());
				}

				static_cast<base_type &>(*this) = std::move(other);

				other.initialize();
				return *this;
			}
		};
		template <typename Data>
		struct container_trivially_move_assignable<Data, true /*move assignable*/, false /*moves allocation*/>
			: container_trivially_move_constructible<Data>
		{
			using base_type = container_trivially_move_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_move_assignable<Data, false /*move assignable*/, false /*moves allocation*/>
			: container_trivially_move_constructible<Data>
		{
			using base_type = container_trivially_move_constructible<Data>;
			using this_type = container_trivially_move_assignable<Data, false, false>;

			using base_type::base_type;

			container_trivially_move_assignable() = default;
			container_trivially_move_assignable(const this_type &) = default;
			container_trivially_move_assignable(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type && other)
			{
				if (this->capacity() < other.size())
				{
					if (this->capacity() > 0)
					{
						this->destruct_range(0, this->size());
						this->deallocate_storage(this->capacity());
					}
					const auto capacity = Data::storage_traits::capacity_for(other.size());
					if (!this->allocate_storage(capacity))
					{
						this->set_capacity(0);
						this->set_size(0);

						return *this;
					}
					this->set_capacity(capacity);
				}
				else if (this->capacity() > 0)
				{
					this->destruct_range(0, this->size());
				}
				this->set_size(other.size()); // x
				this->move_construct_range(0, other, 0, other.size());

				return *this;
			}
		};
	}

	template <typename Data>
	struct container : detail::container_trivially_move_assignable<Data>
	{
		using base_type = detail::container_trivially_move_assignable<Data>;

		container() = default;
		explicit container(std::size_t capacity)
			: base_type(utility::null_place)
		{
			capacity = Data::storage_traits::capacity_for(capacity);
			this->set_capacity(this->allocate_storage(capacity) ? capacity : 0);
			this->set_size(0);
		}

		template <typename Callback>
		bool try_replace_with(std::size_t size, Callback && callback)
		{
			if (this->capacity() < size)
				return try_reallocate_with(Data::storage_traits::capacity_for(size), std::forward<Callback>(callback));

			this->destruct_range(0, this->size());

			this->set_size(size);
			callback(*this, *this); // todo weird
			return true;
		}

		template <typename Callback>
		constexpr bool try_reallocate_with(std::size_t /*capacity*/, Callback && /*callback*/, mpl::true_type /*static capacity*/)
		{
			return false;
		}
		template <typename Callback>
		bool try_reallocate_with(std::size_t capacity, Callback && callback, mpl::false_type /*static capacity*/)
		{
			capacity = Data::storage_traits::capacity_for(capacity);

			Data new_data;
			if (!new_data.allocate_storage(capacity))
				return false;

			new_data.set_capacity(capacity);

			if (!callback(new_data, static_cast<Data &>(*this)))
				return false;

			if (0 < this->capacity())
			{
				this->destruct_range(0, this->size());
				this->deallocate_storage(this->capacity());
			}

			static_cast<Data &>(*this) = std::move(new_data);

			new_data.set_capacity(0);
			new_data.set_size(0);
			return true;
		}
		template <typename Callback>
		constexpr bool try_reallocate_with(std::size_t capacity, Callback && callback)
		{
			return try_reallocate_with(capacity, std::forward<Callback>(callback), typename Data::storage_traits::static_capacity{});
		}

		bool try_reallocate(std::size_t capacity)
		{
			return try_reallocate_with(
				capacity,
				[](Data & new_data, Data & old_data)
				{
					new_data.set_size(old_data.size());
					new_data.move_construct_range(0, old_data, 0, old_data.size());
					return true;
				});
		}

		bool try_reserve(std::size_t count)
		{
			if (count <= this->capacity())
				return true;

			const std::size_t capacity = Data::storage_traits::capacity_for(count);
			if (capacity < count)
				return false;

			return try_reallocate(capacity);
		}
		void reserve(std::size_t count)
		{
			const auto ret = try_reserve(count);
			assert(ret);
		}

		bool try_grow(std::size_t amount = 1)
		{
			if (this->size() + amount <= this->capacity())
				return true;

			const std::size_t capacity = Data::storage_traits::grow(this->capacity(), this->size() + amount - this->capacity());
			if (capacity < this->size() + amount)
				return false;

			return try_reallocate(capacity);
		}
		void grow(std::size_t amount = 1)
		{
			const auto ret = try_grow(amount);
			assert(ret);
		}
	};

	template <typename Storage>
	using array = container<array_data<Storage, initialize_empty>>;
	template <typename Storage>
	using array_nonempty = container<array_data<Storage, initialize_zero>>;
	template <typename Storage>
	using vector = container<vector_data<Storage, initialize_empty>>;

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_array = container<array_data<dynamic_storage<Allocator, Ts...>, initialize_empty>>;
	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_array_nonempty = container<array_data<dynamic_storage<Allocator, Ts...>, initialize_zero>>;
	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_vector = container<vector_data<dynamic_storage<Allocator, Ts...>, initialize_empty>>;

	template <typename ...Ts>
	using heap_array = container<array_data<heap_storage<Ts...>, initialize_empty>>;
	template <typename ...Ts>
	using heap_array_nonempty = container<array_data<heap_storage<Ts...>, initialize_zero>>;
	template <typename ...Ts>
	using heap_vector = container<vector_data<heap_storage<Ts...>, initialize_empty>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_array = container<array_data<static_storage<Capacity, Ts...>, initialize_empty>>;
	template <std::size_t Capacity, typename ...Ts>
	using static_array_nonempty = container<array_data<static_storage<Capacity, Ts...>, initialize_zero>>;
	template <std::size_t Capacity, typename ...Ts>
	using static_vector = container<vector_data<static_storage<Capacity, Ts...>, initialize_empty>>;
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
