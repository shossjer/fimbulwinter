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
				this->purge();
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
				if (this->allocate(other))
				{
					this->copy(other);
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
				if (this->allocate(other))
				{
					this->move(std::move(other));
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
				this->purge();

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
	using basic_container = detail::container_trivially_move_assignable<Data>;
}
