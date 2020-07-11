#pragma once

#include "utility/storage.hpp"
#include "utility/utility.hpp"

namespace utility
{
	template <typename Storage>
	struct initialize_default
	{
		using is_trivial = typename Storage::storing_trivially_default_constructible;

		template <typename Callback>
		void operator () (Callback && callback)
		{
			callback();
		}
	};

	template <typename Storage>
	struct initialize_zero
	{
		using is_trivial = mpl::false_type;

		template <typename Callback>
		void operator () (Callback && callback)
		{
			callback(utility::zero_initialize);
		}
	};

	struct relocate_move
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

		template <typename Data,
		          bool = Data::is_trivially_copy_constructible::value>
		struct container_trivially_copy_constructible
			: container_trivially_destructible<Data>
		{
			using base_type = container_trivially_destructible<Data>;

			using base_type::base_type;
		};

		template <typename Data>
		struct container_trivially_copy_constructible<Data, false /*trivially copy constructible*/>
			: container_trivially_destructible<Data>
		{
			using base_type = container_trivially_destructible<Data>;
			using this_type = container_trivially_copy_constructible<Data, false>;

			using base_type::base_type;

			container_trivially_copy_constructible() = default;
			container_trivially_copy_constructible(const this_type & other)
			{
				if (this->allocate(this->capacity_for(other)))
				{
					this->copy(other);
				}
			}
			container_trivially_copy_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data,
		          bool = Data::is_trivially_copy_assignable::value>
		struct container_trivially_copy_assignable
			: container_trivially_copy_constructible<Data>
		{
			using base_type = container_trivially_copy_constructible<Data>;

			using base_type::base_type;
		};

		template <typename Data>
		struct container_trivially_copy_assignable<Data, false /*trivially copy assignable*/>
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

					if (!this->allocate(this->capacity_for(other)))
						return *this;
				}
				this->copy(other);

				return *this;
			}
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data,
		          bool = Data::is_trivially_move_constructible::value,
		          bool = std::is_move_constructible<Data>::value>
		struct container_trivially_move_constructible
			: container_trivially_copy_assignable<Data>
		{
			using base_type = container_trivially_copy_assignable<Data>;

			using base_type::base_type;
		};

		template <typename Data>
		struct container_trivially_move_constructible<Data, false /*trivially move constructible*/, true /*move constructible*/>
			: container_trivially_copy_assignable<Data>
		{
			using this_type = container_trivially_move_constructible<Data, false, true>;
			using base_type = container_trivially_copy_assignable<Data>;

			using base_type::base_type;

			container_trivially_move_constructible() = default;
			container_trivially_move_constructible(const this_type &) = default;
			container_trivially_move_constructible(this_type && other)
				: base_type(std::move(other))
			{
				this->init(other);

				other.release();
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data>
		struct container_trivially_move_constructible<Data, false /*trivially move constructible*/, false /*move constructible*/>
			: container_trivially_copy_assignable<Data>
		{
			using this_type = container_trivially_move_constructible<Data, false, false>;
			using base_type = container_trivially_copy_assignable<Data>;

			using base_type::base_type;

			container_trivially_move_constructible() = default;
			container_trivially_move_constructible(const this_type &) = default;
			container_trivially_move_constructible(this_type && other)
			{
				if (this->allocate(this->capacity_for(other)))
				{
					this->move(std::move(other));
				}
			}
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};

		template <typename Data,
		          bool = Data::is_trivially_move_assignable::value,
		          bool = std::is_move_assignable<Data>::value>
		struct container_trivially_move_assignable
			: container_trivially_move_constructible<Data>
		{
			using base_type = container_trivially_move_constructible<Data>;

			using base_type::base_type;
		};

		template <typename Data>
		struct container_trivially_move_assignable<Data, false /*trivially move assignable*/, true /*move assignable*/>
			: container_trivially_move_constructible<Data>
		{
			using this_type = container_trivially_move_assignable<Data, false, true>;
			using base_type = container_trivially_move_constructible<Data>;

			using base_type::base_type;

			container_trivially_move_assignable() = default;
			container_trivially_move_assignable(const this_type &) = default;
			container_trivially_move_assignable(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type && other)
			{
				this->purge();

				this->base_type::operator =(std::move(other));

				this->init(other);

				other.release();
				return *this;
			}
		};

		template <typename Data>
		struct container_trivially_move_assignable<Data, false /*trivially move assignable*/, false /*move assignable*/>
			: container_trivially_move_constructible<Data>
		{
			using this_type = container_trivially_move_assignable<Data, false, false>;
			using base_type = container_trivially_move_constructible<Data>;

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

					if (!this->allocate(this->capacity_for(other)))
						return *this;
				}
				this->move(std::move(other));

				return *this;
			}
		};

		template <typename Data, bool = Data::is_trivially_default_constructible::value>
		struct container_trivially_default_constructible
			: container_trivially_move_assignable<Data>
		{
			using base_type = container_trivially_move_assignable<Data>;

			using base_type::base_type;
		};
		template <typename Data>
		struct container_trivially_default_constructible<Data, false /*trivially default constructible*/>
			: container_trivially_move_assignable<Data>
		{
			using base_type = container_trivially_move_assignable<Data>;
			using this_type = container_trivially_default_constructible<Data, false>;

			using base_type::base_type;

			container_trivially_default_constructible()
				: base_type(0)
			{}
			container_trivially_default_constructible(const this_type &) = default;
			container_trivially_default_constructible(this_type &&) = default;
			this_type & operator = (const this_type &) = default;
			this_type & operator = (this_type &&) = default;
		};
	}

	template <typename Data>
	using basic_container = detail::container_trivially_default_constructible<Data>;
}
