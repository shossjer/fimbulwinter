#pragma once

#include "utility/utility.hpp"

namespace utility
{
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
}
