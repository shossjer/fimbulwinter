#pragma once

#include "utility/annotate.hpp"
#include "utility/container/container.hpp"
#include "utility/ranges.hpp"
#include "utility/span.hpp"

namespace utility
{
	namespace detail
	{
		template <typename Storage, bool = utility::storage_traits<Storage>::static_capacity::value>
		class fragmentation_data_impl
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = utility::size_type_for<storage_traits::capacity_value>;

		protected:
			size_type size_;
			typename storage_traits::unpacked storage_;

		public:
			constexpr std::size_t capacity() const { return storage_traits::capacity_value; }

			std::size_t size() const { return size_; }

		protected:
			void set_capacity(std::size_t capacity)
			{
				assert(capacity == 0 || capacity == storage_traits::capacity_value);
				fiw_unused(capacity);
			}

			void set_size(std::size_t size)
			{
				assert(size <= size_type(-1) && size <= storage_traits::capacity_value);

				size_ = static_cast<size_type>(size);
			}
		};

		template <typename Storage>
		class fragmentation_data_impl<Storage, false /*static capacity*/>
		{
			using storage_traits = utility::storage_traits<Storage>;

		public:
			using size_type = std::size_t;

		protected:
			size_type size_;
			size_type capacity_;
			typename storage_traits::unpacked storage_;

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

	template <typename Storage, typename ReservationStrategy, typename RelocationStrategy,
	          typename ModedStorage = typename utility::storage_traits<Storage>::template append<std::size_t>>
	class fragmented_data
		: public detail::fragmentation_data_impl<ModedStorage>
	{
		using this_type = fragmented_data<Storage, ReservationStrategy, RelocationStrategy, ModedStorage>;
		using base_type = detail::fragmentation_data_impl<ModedStorage>;

		using StorageTraits = utility::storage_traits<ModedStorage>;

	public:
		using storage_type = Storage;

		using is_trivially_destructible =
			mpl::conjunction<typename Storage::storing_trivially_destructible,
			                 typename StorageTraits::trivial_deallocate>;

		using is_trivially_default_constructible = mpl::false_type;

		using is_trivially_copy_constructible = std::is_trivially_copy_constructible<Storage>;
		using is_trivially_copy_assignable = std::is_trivially_copy_constructible<Storage>;
		using is_trivially_move_constructible = std::is_trivially_move_constructible<Storage>;
		using is_trivially_move_assignable = std::is_trivially_move_constructible<Storage>;

	public:
		auto begin_indices() { return this->storage_.begin_for(mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto begin_indices() const { return this->storage_.begin_for(mpl::index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		auto end_indices() { return begin_indices() + this->capacity(); }
		auto end_indices() const { return begin_indices() + this->capacity(); }

		auto begin_storage() { return this->storage_.begin_for(mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }
		auto begin_storage() const { return this->storage_.begin_for(mpl::make_index_sequence<(utility::storage_size<ModedStorage>::value - 1)>{}); }

		void copy(const this_type & other)
		{
			this->set_size(other.size());

			const auto end_indices = this->storage_.memcpy_range(begin_indices(), other.storage_.data(other.begin_indices()), other.storage_.data(other.end_indices()));
			auto remaining_indices = ranges::index_sequence(other.capacity(), this->capacity());
			this->storage_.construct_range(end_indices, remaining_indices.begin(), remaining_indices.end());

			for (auto index : utility::span<std::size_t>(this->storage_.data(begin_indices()), this->size()))
			{
				this->storage_.construct_at_(begin_storage() + index, other.storage_.data(other.begin_storage())[index]);
			}
		}

		void move(this_type && other)
		{
			this->set_size(other.size());

			const auto end_indices = this->storage_.memcpy_range(begin_indices(), other.storage_.data(other.begin_indices()), other.storage_.data(other.end_indices()));
			auto remaining_indices = ranges::index_sequence(other.capacity(), this->capacity());
			this->storage_.construct_range(end_indices, remaining_indices.begin(), remaining_indices.end());

			for (auto index : utility::span<std::size_t>(this->storage_.data(begin_indices()), this->size()))
			{
				using utility::iter_move;
				this->storage_.construct_at_(begin_storage() + index, iter_move(other.storage_.data(other.begin_storage() + index)));
			}
		}

	protected:
		fragmented_data() = default;

		explicit fragmented_data(std::size_t min_capacity)
		{
			if (allocate(ReservationStrategy{}(min_capacity)))
			{
				this->set_size(0);

				this->storage_.construct_range(begin_indices(), ranges::iota_iterator<ext::index>(0), ranges::iota_iterator<ext::index>(this->capacity()));
			}
		}

		void init(const this_type & /*other*/)
		{
		}

		void release()
		{
			if (StorageTraits::moves_allocation::value)
			{
				this->set_capacity(0);
				this->set_size(0);
			}
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
			for (auto index : utility::span<std::size_t>(this->storage_.data(begin_indices()), this->size()))
			{
				this->storage_.destruct_at(begin_storage() + index);
			}
			// this->storage_.destruct_range(begin_indices(), end_indices()); // note trivial
		}

		void purge()
		{
			if (this->storage_.good())
			{
				for (auto index : utility::span<std::size_t>(this->storage_.data(begin_indices()), this->size()))
				{
					this->storage_.destruct_at(begin_storage() + index);
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

		annotate_nodiscard
		constexpr std::size_t capacity() const { return base_type::capacity(); }
		annotate_nodiscard
		std::size_t size() const { return base_type::size(); }

		annotate_nodiscard
		auto data() { return this->storage_.data(this->begin_storage()); }
		annotate_nodiscard
		auto data() const { return this->storage_.data(this->begin_storage()); }

		annotate_nodiscard
		reference operator [] (ext::index index) { return data()[index]; }
		annotate_nodiscard
		const_reference operator [] (ext::index index) const { return data()[index]; }

		annotate_nodiscard
		bool try_reserve(std::size_t min_capacity)
		{
			if (min_capacity <= this->capacity())
				return true;

			return this->try_reallocate(min_capacity);
		}

		template <typename ...Ps>
		annotate_nodiscard
		pointer try_emplace(Ps && ...ps)
		{
			if (!this->try_reserve(this->size() + 1))
				return nullptr;

			const auto index = this->storage_.data(this->begin_indices())[this->size()];

			auto && e = this->storage_.construct_at_(this->begin_storage() + index, std::forward<Ps>(ps)...);
			this->set_size(this->size() + 1);
			return std::pointer_traits<pointer>::pointer_to(e);
		}

		annotate_nodiscard
		bool try_erase(ext::index index)
		{
			if (!/*debug_assert*/(std::find(this->storage_.data(this->begin_indices()) + this->size(), this->storage_.data(this->begin_indices()) + this->capacity(), static_cast<std::size_t>(index)) == this->storage_.data(this->begin_indices()) + this->capacity()))
				return false;

			const auto last = this->size() - 1;

			this->storage_.destruct_at(this->begin_storage() + index);
			this->storage_.data(this->begin_indices())[last] = index; // todo swap?
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
