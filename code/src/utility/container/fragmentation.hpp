#pragma once

#include "utility/ranges.hpp"
#include "utility/span.hpp"
#include "utility/storage.hpp"

#include "utility/array_alloc.hpp"

namespace utility
{
	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class fragmented_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = utility::size_type_for<storage_traits::capacity_value>;

		protected:
			std::size_t size_;
			Storage storage_;

		protected:
			fragmented_data_impl()
				: size_{}
			{}
			explicit fragmented_data_impl(utility::null_place_t) {}

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
		class fragmented_data_impl<Storage, false /*static capacity*/>
		{
		public:
			using size_type = std::size_t;

		protected:
			std::size_t size_;
			std::size_t capacity_;
			Storage storage_;

		protected:
			fragmented_data_impl()
				: size_{}
				, capacity_{}
			{}
			explicit fragmented_data_impl(utility::null_place_t) {}

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

	template <typename Storage, typename InitializationStrategy,
	          typename ModedStorage = typename utility::storage_traits<Storage>::template append<std::size_t>>
	class fragmented_data
		: public detail::fragmented_data_impl<ModedStorage>
	{
		using this_type = fragmented_data<Storage, InitializationStrategy, ModedStorage>;
		using base_type = detail::fragmented_data_impl<ModedStorage>;

		friend InitializationStrategy;

	public:
		using storage_type = Storage;

		using storage_traits = utility::storage_traits<ModedStorage>;

		// todo remove these
		using is_trivially_destructible = utility::storage_is_trivially_destructible<ModedStorage>;
		using is_trivially_default_constructible =
			mpl::conjunction<typename storage_traits::static_capacity,
			                 utility::storage_is_trivially_default_constructible<ModedStorage>>;

	protected:
		auto storage() { return this->storage_.sections_for(this->capacity(), mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto storage() const { return this->storage_.sections_for(this->capacity(), mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		auto indices() { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto indices() const { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		using base_type::base_type;

		fragmented_data() // todo combine with trivial default constructor?
			: base_type()
		{
			this->initialize();
		}

		bool allocate(const this_type & other)
		{
			const auto capacity = other.capacity();
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

		void initialize()
		{
			InitializationStrategy{}(*this);
		}

		bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.capacity());
		}

		void clear()
		{
			if (0 < this->capacity())
			{
				for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
				{
					storage().destruct_at(index);
				}
				// indices().destruct_range(0, this->capacity()); // note trivial
			}
		}

		void purge()
		{
			if (0 < this->capacity())
			{
				for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
				{
					storage().destruct_at(index);
				}
				// indices().destruct_range(0, this->capacity()); // note trivial
				this->storage_.deallocate(this->capacity());
			}
		}

		void copy(const this_type & other)
		{
			this->set_size(other.size());

			indices().memcpy_range(0, other.indices().data(), other.indices().data() + other.capacity());
			auto remaining_indices = ranges::index_sequence(other.capacity(), this->capacity());
			indices().construct_range(other.capacity(), remaining_indices.begin(), remaining_indices.end());

			for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
			{
				storage().construct_at(index, other.storage()[index]);
			}
		}

		void move(this_type && other)
		{
			this->set_size(other.size());

			indices().memcpy_range(0, other.indices().data(), other.indices().data() + other.capacity());
			auto remaining_indices = ranges::index_sequence(other.capacity(), this->capacity());
			indices().construct_range(other.capacity(), remaining_indices.begin(), remaining_indices.end());

			for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
			{
				storage().construct_at(index, std::move(other.storage()[index]));
			}
		}

	private:
		void initialize_empty()
		{
			if (0 < this->capacity())
			{
				indices().construct_range(0, ranges::iota_iterator<ext::index>(0), ranges::iota_iterator<ext::index>(this->capacity()));
			}
		}

		template <typename ...Ps>
		void initialize_fill(Ps && ...ps)
		{
			storage().construct_fill(0, this->capacity(), std::forward<Ps>(ps)...);
			indices().construct_range(0, ranges::iota_iterator<ext::index>(0), ranges::iota_iterator<ext::index>(this->capacity()));
		}
	};

	namespace detail
	{
		template <typename Data, bool = Data::is_trivially_destructible::value>
		struct fragmentation_trivially_destructible
			: Data
		{
			using base_type = Data;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_destructible<Data, false /*trivially destructible*/>
			: Data
		{
			using base_type = Data;
			using this_type = fragmentation_trivially_destructible<Data, false>;

			using base_type::base_type;

			~fragmentation_trivially_destructible()
			{
				this->purge();
			}
			fragmentation_trivially_destructible() = default;
			fragmentation_trivially_destructible(const this_type &) = default;
			fragmentation_trivially_destructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = Data::is_trivially_default_constructible::value>
		struct fragmentation_trivially_default_constructible
			: fragmentation_trivially_destructible<Data>
		{
			using base_type = fragmentation_trivially_destructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_default_constructible<Data, false /*trivially default constructible*/>
			: fragmentation_trivially_destructible<Data>
		{
			using base_type = fragmentation_trivially_destructible<Data>;
			using this_type = fragmentation_trivially_default_constructible<Data, false>;

			using base_type::base_type;

			fragmentation_trivially_default_constructible()
				: base_type(utility::null_place)
			{
				this->initialize();
			}
			fragmentation_trivially_default_constructible(const this_type &) = default;
			fragmentation_trivially_default_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = std::is_copy_constructible<Data>::value>
		struct fragmentation_trivially_copy_constructible
			: fragmentation_trivially_default_constructible<Data>
		{
			using base_type = fragmentation_trivially_default_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_copy_constructible<Data, false /*copy constructible*/>
			: fragmentation_trivially_default_constructible<Data>
		{
			using base_type = fragmentation_trivially_default_constructible<Data>;
			using this_type = fragmentation_trivially_copy_constructible<Data, false>;

			using base_type::base_type;

			fragmentation_trivially_copy_constructible() = default;
			fragmentation_trivially_copy_constructible(const this_type & other)
				: base_type(utility::null_place)
			{
				if (this->allocate(other))
				{
					this->copy(other);
				}
			}
			fragmentation_trivially_copy_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool = std::is_copy_assignable<Data>::value>
		struct fragmentation_trivially_copy_assignable
			: fragmentation_trivially_copy_constructible<Data>
		{
			using base_type = fragmentation_trivially_copy_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_copy_assignable<Data, false /*copy assignable*/>
			: fragmentation_trivially_copy_constructible<Data>
		{
			using base_type = fragmentation_trivially_copy_constructible<Data>;
			using this_type = fragmentation_trivially_copy_assignable<Data, false>;

			using base_type::base_type;

			fragmentation_trivially_copy_assignable() = default;
			fragmentation_trivially_copy_assignable(const this_type &) = default;
			fragmentation_trivially_copy_assignable(this_type &&) = default;
			this_type & operator = (const this_type & other)
			{
				if (this->fits(other))
				{
					this->clear();
				}
				else
				{
					this->purge();

					if (!this->allocate(other))
						return *this;
				}
				this->copy(other);

				return *this;
			}
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool move_constructible = std::is_move_constructible<Data>::value, bool moves_allocation = Data::storage_traits::moves_allocation::value>
		struct fragmentation_trivially_move_constructible
			: fragmentation_trivially_copy_assignable<Data>
		{
			static_assert(move_constructible && moves_allocation, "");

			using base_type = fragmentation_trivially_copy_assignable<Data>;
			using this_type = fragmentation_trivially_move_constructible<Data, move_constructible, moves_allocation>;

			using base_type::base_type;

			fragmentation_trivially_move_constructible() = default;
			fragmentation_trivially_move_constructible(const this_type &) = default;
			fragmentation_trivially_move_constructible(this_type && other)
				: base_type(std::move(other))
			{
				other.initialize();
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};
		template <typename Data>
		struct fragmentation_trivially_move_constructible<Data, true /*move constructible*/, false /*moves allocation*/>
			: fragmentation_trivially_copy_assignable<Data>
		{
			using base_type = fragmentation_trivially_copy_assignable<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_move_constructible<Data, false /*move constructible*/, false /*moves allocation*/>
			: fragmentation_trivially_copy_assignable<Data>
		{
			using base_type = fragmentation_trivially_copy_assignable<Data>;
			using this_type = fragmentation_trivially_move_constructible<Data, false, false>;

			using base_type::base_type;

			fragmentation_trivially_move_constructible() = default;
			fragmentation_trivially_move_constructible(const this_type &) = default;
			fragmentation_trivially_move_constructible(this_type && other)
				: base_type(utility::null_place)
			{
				if (this->allocate(other))
				{
					this->move(std::move(other));
				}
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data, bool move_assignable = std::is_move_assignable<Data>::value, bool moves_allocation = Data::storage_traits::moves_allocation::value>
		struct fragmentation_trivially_move_assignable
			: fragmentation_trivially_move_constructible<Data>
		{
			static_assert(move_assignable && moves_allocation, "");

			using base_type = fragmentation_trivially_move_constructible<Data>;
			using this_type = fragmentation_trivially_move_assignable<Data, move_assignable, moves_allocation>;

			using base_type::base_type;

			fragmentation_trivially_move_assignable() = default;
			fragmentation_trivially_move_assignable(const this_type &) = default;
			fragmentation_trivially_move_assignable(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type && other)
			{
				this->purge();

				static_cast<base_type &>(*this) = std::move(other);

				other.initialize();
				return *this;
			}
		};
		template <typename Data>
		struct fragmentation_trivially_move_assignable<Data, true /*move assignable*/, false /*moves allocation*/>
			: fragmentation_trivially_move_constructible<Data>
		{
			using base_type = fragmentation_trivially_move_constructible<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct fragmentation_trivially_move_assignable<Data, false /*move assignable*/, false /*moves allocation*/>
			: fragmentation_trivially_move_constructible<Data>
		{
			using base_type = fragmentation_trivially_move_constructible<Data>;
			using this_type = fragmentation_trivially_move_assignable<Data, false, false>;

			using base_type::base_type;

			fragmentation_trivially_move_assignable() = default;
			fragmentation_trivially_move_assignable(const this_type &) = default;
			fragmentation_trivially_move_assignable(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type && other)
			{
				if (this->fits(other))
				{
					this->clear();
				}
				else
				{
					this->purge();

					if (!this->allocate(other))
						return *this;
				}
				this->move(std::move(other));

				return *this;
			}
		};
	}

	template <typename Data>
	class basic_fragmentation
		: public detail::fragmentation_trivially_move_assignable<Data>
	{
		using base_type = detail::fragmentation_trivially_move_assignable<Data>;

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
		pointer data() { return this->storage().data(); }
		const_pointer data() const { return this->storage().data(); }

		reference operator [] (ext::index index) { return this->storage().data()[index]; }
		const_reference operator [] (ext::index index) const { return this->storage().data()[index]; }

		template <typename ...Ps>
		pointer try_emplace(Ps && ...ps)
		{
			if (!/*debug_assert*/(this->size() < this->capacity()))
				return nullptr;

			const auto index = this->indices()[this->size()];

			auto & e = this->storage().construct_at(index, std::forward<Ps>(ps)...);
			this->set_size(this->size() + 1);
			return &e;
		}

		bool try_erase(ext::index index)
		{
			if (!/*debug_assert*/(std::find(this->indices().data() + this->size(), this->indices().data() + this->capacity(), index) == this->indices().data() + this->capacity()))
				return false;

			const auto last = this->size() - 1;

			this->storage().destruct_at(index);
			this->indices()[last] = index; // todo swap?
			this->set_size(last);
			return true;
		}
	};

	template <typename Storage>
	using fragmentation = basic_fragmentation<fragmented_data<Storage, utility::initialize_empty>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_fragmentation = basic_fragmentation<fragmented_data<utility::static_storage<Capacity, Ts...>, utility::initialize_empty>>;
	template <typename ...Ts>
	using heap_fragmentation = basic_fragmentation<fragmented_data<utility::heap_storage<Ts...>, utility::initialize_empty>>;
}
