
#ifndef UTILITY_ARRAY_ALLOC_HPP
#define UTILITY_ARRAY_ALLOC_HPP

#include "storage.hpp"
#include "utility.hpp"

#include <cassert>

namespace utility
{
	template <std::size_t N>
	using size_type_for =
		mpl::conditional_t<(N < 0x100), std::uint8_t,
		mpl::conditional_t<(N < 0x10000), std::uint16_t,
		mpl::conditional_t<(N < 0x100000000), std::uint32_t, std::uint64_t>>>;

	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		struct array_data_impl;

		template <typename Storage>
		struct array_data_impl<Storage, true /*static capacity*/>
		{
			using storage_traits = utility::storage_traits<Storage>;

			using size_type = utility::size_type_for<storage_traits::capacity_value>;

			size_type size_ = 0;
			Storage storage_;

			void set_capacity(std::size_t capacity)
			{
				assert(capacity == storage_traits::capacity_value);
				static_cast<void>(capacity);
			}

			void set_size(std::size_t size)
			{
				assert(size <= size_type(-1));

				size_ = static_cast<size_type>(size);
			}

			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }
		};

		template <typename Storage>
		struct array_data_impl<Storage, false /*static capacity*/>
		{
			using size_type = std::size_t;
			using storage_traits = utility::storage_traits<Storage>;

			size_type size_ = 0;
			size_type capacity_ = 0;
			Storage storage_;

			void set_capacity(std::size_t capacity)
			{
				capacity_ = capacity;
			}

			void set_size(std::size_t size)
			{
				size_ = size;
			}

			std::size_t capacity() const { return capacity_; }
		};
	}

	template <typename Storage>
	struct array_data
		: detail::array_data_impl<Storage>
	{
		using is_trivially_destructible = storage_is_trivially_destructible<Storage>;
		using is_trivially_copy_constructible = storage_is_copy_constructible<Storage>;
		using is_trivially_copy_assignable = storage_is_copy_assignable<Storage>;
		using is_trivially_move_constructible = storage_is_trivially_move_constructible<Storage>;
		using is_trivially_move_assignable = storage_is_trivially_move_assignable<Storage>;

		using this_type = array_data<Storage>;

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
			this->set_capacity(0);
			this->set_size(0);
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

		std::size_t size() const { return this->size_; }
	};

	namespace detail
	{
		template <typename StorageData, bool = StorageData::is_trivially_destructible::value>
		struct array_wrapper_trivially_destructible
			: StorageData
		{};
		template <typename StorageData>
		struct array_wrapper_trivially_destructible<StorageData, false /*trivially destructible*/>
			: StorageData
		{
			using base_type = StorageData;
			using this_type = array_wrapper_trivially_destructible<StorageData, false>;

			using base_type::base_type;

			~array_wrapper_trivially_destructible()
			{
				if (this->capacity() > 0)
				{
					this->destruct_range(0, this->size());
					this->deallocate_storage(this->capacity());
				}
			}
			array_wrapper_trivially_destructible() = default;
			array_wrapper_trivially_destructible(const this_type &) = default;
			array_wrapper_trivially_destructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename StorageData, bool = StorageData::is_trivially_copy_constructible::value>
		struct array_wrapper_trivially_copy_constructible
			: array_wrapper_trivially_destructible<StorageData>
		{};
		template <typename StorageData>
		struct array_wrapper_trivially_copy_constructible<StorageData, false /*copy constructible*/>
			: array_wrapper_trivially_destructible<StorageData>
		{
			using base_type = array_wrapper_trivially_destructible<StorageData>;
			using this_type = array_wrapper_trivially_copy_constructible<StorageData, false>;

			using base_type::base_type;

			array_wrapper_trivially_copy_constructible() = default;
			array_wrapper_trivially_copy_constructible(const this_type & other)
			{
				const auto capacity = StorageData::storage_traits::capacity_for(other.size());
				if (this->allocate_storage(capacity))
				{
					this->set_capacity(capacity);

					this->set_size(other.size());
					this->copy_construct_range(0, other, 0, other.size());
				}
			}
			array_wrapper_trivially_copy_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename StorageData, bool = StorageData::is_trivially_copy_assignable::value>
		struct array_wrapper_trivially_copy_assignable
			: array_wrapper_trivially_copy_constructible<StorageData>
		{};
		template <typename StorageData>
		struct array_wrapper_trivially_copy_assignable<StorageData, false /*copy assignable*/>
			: array_wrapper_trivially_copy_constructible<StorageData>
		{
			using base_type = array_wrapper_trivially_copy_constructible<StorageData>;
			using this_type = array_wrapper_trivially_copy_assignable<StorageData, false>;

			using base_type::base_type;

			array_wrapper_trivially_copy_assignable() = default;
			array_wrapper_trivially_copy_assignable(const this_type &) = default;
			array_wrapper_trivially_copy_assignable(this_type &&) = default;
			this_type & operator = (const this_type & other)
			{
				if (this->capacity() < other.size())
				{
					if (this->capacity() > 0)
					{
						this->destruct_range(0, this->size());
						this->deallocate_storage(this->capacity());
					}
					const auto capacity = StorageData::storage_traits::capacity_for(other.size());
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

		template <typename StorageData, bool = StorageData::is_trivially_move_constructible::value, bool = StorageData::storage_traits::moves_allocation::value>
		struct array_wrapper_trivially_move_constructible
			: array_wrapper_trivially_copy_assignable<StorageData>
		{};
		template <typename StorageData>
		struct array_wrapper_trivially_move_constructible<StorageData, false /*trivially move constructible*/, true /*moves allocation*/>
			: array_wrapper_trivially_copy_assignable<StorageData>
		{
			using base_type = array_wrapper_trivially_copy_assignable<StorageData>;
			using this_type = array_wrapper_trivially_move_constructible<StorageData, false, true>;

			using base_type::base_type;

			array_wrapper_trivially_move_constructible() = default;
			array_wrapper_trivially_move_constructible(const this_type &) = default;
			array_wrapper_trivially_move_constructible(this_type && other)
				: base_type(std::move(other))
			{
				other.initialize();
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};
		template <typename StorageData>
		struct array_wrapper_trivially_move_constructible<StorageData, false /*trivially move constructible*/, false /*moves allocation*/>
			: array_wrapper_trivially_copy_assignable<StorageData>
		{
			using base_type = array_wrapper_trivially_copy_assignable<StorageData>;
			using this_type = array_wrapper_trivially_move_constructible<StorageData, false, false>;

			using base_type::base_type;

			array_wrapper_trivially_move_constructible() = default;
			array_wrapper_trivially_move_constructible(const this_type &) = default;
			array_wrapper_trivially_move_constructible(this_type && other)
			{
				const auto capacity = StorageData::storage_traits::capacity_for(other.size());
				if (this->allocate_storage(capacity))
				{
					this->set_capacity(capacity);

					this->set_size(other.size());
					this->move_construct_range(0, other, 0, other.size());
				}
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename StorageData, bool = StorageData::is_trivially_move_assignable::value, bool = StorageData::storage_traits::moves_allocation::value>
		struct array_wrapper_trivially_move_assignable
			: array_wrapper_trivially_move_constructible<StorageData>
		{};
		template <typename StorageData>
		struct array_wrapper_trivially_move_assignable<StorageData, false /*trivially move assignable*/, true /*moves allocation*/>
			: array_wrapper_trivially_move_constructible<StorageData>
		{
			using base_type = array_wrapper_trivially_move_constructible<StorageData>;
			using this_type = array_wrapper_trivially_move_assignable<StorageData, false, true>;

			using base_type::base_type;

			array_wrapper_trivially_move_assignable() = default;
			array_wrapper_trivially_move_assignable(const this_type &) = default;
			array_wrapper_trivially_move_assignable(this_type &&) = default;
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
		template <typename StorageData>
		struct array_wrapper_trivially_move_assignable<StorageData, false /*trivially move assignable*/, false /*moves allocation*/>
			: array_wrapper_trivially_move_constructible<StorageData>
		{
			using base_type = array_wrapper_trivially_move_constructible<StorageData>;
			using this_type = array_wrapper_trivially_move_assignable<StorageData, false, false>;

			using base_type::base_type;

			array_wrapper_trivially_move_assignable() = default;
			array_wrapper_trivially_move_assignable(const this_type &) = default;
			array_wrapper_trivially_move_assignable(this_type &&) = default;
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
					const auto capacity = StorageData::storage_traits::capacity_for(other.size());
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

	template <typename StorageData>
	struct array_wrapper : detail::array_wrapper_trivially_move_assignable<StorageData>
	{
		array_wrapper() = default;
		array_wrapper(std::size_t capacity)
		{
			capacity = StorageData::storage_traits::capacity_for(capacity);
			if (this->allocate_storage(capacity))
			{
				this->set_capacity(capacity);
			}
		}

		template <typename Callback>
		bool try_replace_with(std::size_t size, Callback && callback)
		{
			if (this->capacity() < size)
				return try_reallocate_with(StorageData::storage_traits::capacity_for(size), std::forward<Callback>(callback));

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
			StorageData new_data;
			new_data.set_capacity(capacity);
			if (!new_data.allocate_storage(capacity))
				return false;

			if (!callback(new_data, static_cast<StorageData &>(*this)))
				return false;

			if (this->capacity() > 0)
			{
				this->destruct_range(0, this->size());
				this->deallocate_storage(this->capacity());
			}

			static_cast<StorageData &>(*this) = std::move(new_data);

			new_data.set_capacity(0);
			new_data.set_size(0);
			return true;
		}
		template <typename Callback>
		constexpr bool try_reallocate_with(std::size_t capacity, Callback && callback)
		{
			return try_reallocate_with(capacity, std::forward<Callback>(callback), typename StorageData::storage_traits::static_capacity{});
		}

		bool try_reallocate(std::size_t capacity)
		{
			return try_reallocate_with(
				capacity,
				[](StorageData & new_data, StorageData & old_data)
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

			const std::size_t capacity = StorageData::storage_traits::capacity_for(count);
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

			const std::size_t capacity = StorageData::storage_traits::grow(this->capacity(), this->size() + amount - this->capacity());
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
}

#endif /* UTILITY_ARRAY_ALLOC_HPP */
