#pragma once

#include "utility/container/vector.hpp"
#include "utility/ranges.hpp"
#include "utility/span.hpp"

namespace utility
{
	template <typename Storage, typename ReservationStrategy, typename RelocationStrategy,
	          typename ModedStorage = typename utility::storage_traits<Storage>::template append<std::size_t>>
	class fragmented_data
		: public detail::vector_data_impl<ModedStorage>
	{
		using this_type = fragmented_data<Storage, ReservationStrategy, RelocationStrategy, ModedStorage>;
		using base_type = detail::vector_data_impl<ModedStorage>;

		using StorageTraits = utility::storage_traits<ModedStorage>;

	public:
		using storage_type = Storage;

		using is_trivially_destructible =
			mpl::conjunction<typename Storage::storing_trivially_destructible,
			                 typename StorageTraits::trivial_deallocate>;

		using is_trivially_default_constructible = mpl::false_type;

	public:
		auto storage() { return this->storage_.sections_for(this->capacity(), mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto storage() const { return this->storage_.sections_for(this->capacity(), mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		auto indices() { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto indices() const { return this->storage_.sections_for(this->capacity(), mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		void copy(const this_type & other)
		{
			this->set_size(other.size());

			indices().memcpy_range(0, other.indices().data() + 0, other.indices().data() + other.capacity());
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

			indices().memcpy_range(0, other.indices().data() + 0, other.indices().data() + other.capacity());
			auto remaining_indices = ranges::index_sequence(other.capacity(), this->capacity());
			indices().construct_range(other.capacity(), remaining_indices.begin(), remaining_indices.end());

			for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
			{
				storage().construct_at(index, std::move(other.storage()[index]));
			}
		}

	protected:
		fragmented_data() = default;

		explicit fragmented_data(std::size_t min_capacity)
		{
			if (allocate(ReservationStrategy{}(min_capacity)))
			{
				this->set_size(0);

				indices().construct_range(0, ranges::iota_iterator<ext::index>(0), ranges::iota_iterator<ext::index>(this->capacity()));
			}
		}

		void release()
		{
			this->set_capacity(0);
			this->set_size(0);
		}

		bool allocate(std::size_t capacity)
		{
			if (this->storage_.allocate(capacity))
			{
				this->set_capacity(capacity);
				return true;
			}
			else
			{
				release();
				return false;
			}
		}

		bool fits(const this_type & other) const
		{
			return !(this->capacity() < other.capacity());
		}

		void clear()
		{
			for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
			{
				storage().destruct_at(index);
			}
			// indices().destruct_range(0, this->capacity()); // note trivial
		}

		void purge()
		{
			if (this->storage_.good())
			{
				for (auto index : utility::span<std::size_t>(indices().data(), this->size()))
				{
					storage().destruct_at(index);
				}
				// indices().destruct_range(0, this->capacity()); // note trivial
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
	class basic_fragmentation
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
		basic_fragmentation() = default;

		explicit basic_fragmentation(std::size_t size)
			: base_type(size)
		{}

		constexpr std::size_t capacity() const { return base_type::capacity(); }
		std::size_t size() const { return base_type::size(); }

		pointer data() { return this->storage().data(); }
		const_pointer data() const { return this->storage().data(); }

		reference operator [] (ext::index index) { return this->storage().data()[index]; }
		const_reference operator [] (ext::index index) const { return this->storage().data()[index]; }

		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		template <typename ...Ps>
		pointer try_emplace(Ps && ...ps)
		{
			if (!this->try_reserve(this->size() + 1))
				return nullptr;

			const auto index = this->indices()[this->size()];

			auto && e = this->storage().construct_at(index, std::forward<Ps>(ps)...);
			this->set_size(this->size() + 1);
			return std::pointer_traits<pointer>::pointer_to(e);
		}

		bool try_erase(ext::index index)
		{
			if (!/*debug_assert*/(std::find(this->indices().data() + this->size(), this->indices().data() + this->capacity(), static_cast<std::size_t>(index)) == this->indices().data() + this->capacity()))
				return false;

			const auto last = this->size() - 1;

			this->storage().destruct_at(index);
			this->indices()[last] = index; // todo swap?
			this->set_size(last);
			return true;
		}
	};

	template <typename Storage,
	          template <typename> class ReservationStrategy = utility::reserve_power_of_two,
	          typename RelocationStrategy = utility::relocate_move>
	using fragmentation = basic_fragmentation<fragmented_data<Storage,
	                                                          ReservationStrategy<Storage>,
	                                                          RelocationStrategy>>;

	template <std::size_t Capacity, typename ...Ts>
	using static_fragmentation = fragmentation<utility::static_storage<Capacity, Ts...>>;
	template <typename ...Ts>
	using heap_fragmentation = fragmentation<utility::heap_storage<Ts...>>;
}
