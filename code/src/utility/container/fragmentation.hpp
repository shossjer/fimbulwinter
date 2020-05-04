#pragma once

#include "utility/container/vector.hpp"
#include "utility/ranges.hpp"
#include "utility/span.hpp"

namespace utility
{
	template <typename Storage, typename InitializationStrategy,
	          typename ModedStorage = typename utility::storage_traits<Storage>::template append<std::size_t>>
	class fragmented_data
		: public detail::vector_data_impl<ModedStorage>
	{
		using this_type = fragmented_data<Storage, InitializationStrategy, ModedStorage>;
		using base_type = detail::vector_data_impl<ModedStorage>;

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

	template <typename Data>
	class basic_fragmentation
		: public basic_container<Data>
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
