#pragma once

#include "config.h"

#include "utility/algorithm.hpp"
#include "utility/aggregation_allocator.hpp"
#include "utility/bitmanip.hpp"
#include "utility/compound.hpp"
#include "utility/ext/stddef.hpp"
#include "utility/heap_allocator.hpp"
#include "utility/iterator.hpp"
#include "utility/null_allocator.hpp"
#include "utility/storing.hpp"
#include "utility/tuple.hpp"

#include <array>
#include <cassert>
#include <cstring>

namespace utility
{
	struct zero_initialize_t
	{
		explicit zero_initialize_t() = default;
	};
	constexpr zero_initialize_t zero_initialize = zero_initialize_t{};

	template <typename Storage, std::size_t ...Is>
	class storage_data
	{
		using value_types = mpl::type_list<typename Storage::template value_type_at<Is>...>;
		using const_value_types = mpl::transform<std::add_const_t, value_types>;
		using storing_types = mpl::transform<Storage::template storing_type_for, value_types>;

		using pointers = mpl::transform<std::add_pointer_t, value_types>;
		using const_pointers = mpl::transform<std::add_pointer_t, const_value_types>;
		using references = mpl::transform<std::add_lvalue_reference_t, value_types>;
		using const_references = mpl::transform<std::add_lvalue_reference_t, const_value_types>;

		using iterator = mpl::apply<utility::zip_iterator,
		                            mpl::transform<std::add_pointer_t,
		                                           storing_types>>;

		template <std::size_t I>
		using value_type_i = typename Storage::template value_type_at<I>;
		template <std::size_t I>
		using storing_type_i = typename Storage::template storing_type_for<value_type_i<I>>;

		template <std::size_t K>
		using value_type_k = mpl::type_at<K, value_type_i<Is>...>;
		template <std::size_t K>
		using storing_type_k = typename Storage::template storing_type_for<value_type_k<K>>;

		template <std::size_t K, typename InputIt>
		using can_memcpy_k = mpl::conjunction<std::is_trivially_copyable<storing_type_k<K>>,
		                                       utility::is_contiguous_iterator<InputIt>>;
		template <std::size_t K>
		using can_memset_k = std::is_trivially_copyable<storing_type_k<K>>;

	public:
		using value_type = utility::combine<utility::compound, value_types>;
		using reference = utility::combine<utility::compound, references>;
		using const_reference = utility::combine<utility::compound, const_references>;
		using pointer = utility::combine<utility::zip_pointer, pointers>;
		using const_pointer = utility::combine<utility::zip_pointer, const_pointers>;

		template <typename InputIt>
		using can_memcpy = mpl::conjunction<std::is_trivially_copyable<storing_type_i<Is>>...,
		                                    utility::is_contiguous_iterator<InputIt>>;

		using can_memset = mpl::conjunction<std::is_trivially_copyable<storing_type_i<Is>>...>;

	private:
		Storage * storage_;
		iterator it_;

	public:
		template <typename ...Ps>
		explicit storage_data(Storage & storage, Ps && ...ps)
			: storage_(&storage)
			, it_(std::forward<Ps>(ps)...)
		{}

	public:
		reference operator [] (ext::index index)
		{
			return ext::apply([&](auto & ...ps) -> reference { return reference(*storage_->data(ps + index)...); }, it_);
		}

		const_reference operator [] (ext::index index) const
		{
			return ext::apply([&](auto & ...ps) -> const_reference { return const_reference(*storage_->data(ps + index)...); }, it_);
		}

		pointer data()
		{
			return ext::apply([&](auto & ...ps){ return pointer(storage_->data(ps)...); }, it_);
		}
		const_pointer data() const
		{
			return ext::apply([&](auto & ...ps){ return const_pointer(storage_->data(ps)...); }, it_);
		}

		template <typename ...Ps>
		void construct_fill(ext::index begin, ext::index end, Ps && ...ps)
		{
			return construct_fill_impl(mpl::make_index_sequence<sizeof...(Is)>{}, begin, end, std::forward<Ps>(ps)...);
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(mpl::make_index_sequence<sizeof...(Is)>{}, index, begin, end);
		}

		template <typename ...Ps>
		reference construct_at(ext::index index, Ps && ...ps)
		{
			return construct_at_impl(mpl::make_index_sequence<sizeof...(Is)>{}, index, std::forward<Ps>(ps)...);
		}

		template <typename InputIt,
		          REQUIRES((can_memcpy<InputIt>::value))>
		void memcpy_range(ext::index index, InputIt begin, InputIt end)
		{
			memcpy_range_impl(mpl::make_index_sequence<sizeof...(Is)>{}, index, begin, end);
		}

		template <typename can_memset_ = can_memset,
		          REQUIRES((can_memset_::value))>
		void memset_fill(ext::index begin, ext::index end, ext::byte value)
		{
			// todo everything at once when begin and end is the whole range
			memset_fill_impl(mpl::make_index_sequence<sizeof...(Is)>{}, begin, end, value);
		}

		void destruct_range(ext::index begin, ext::index end)
		{
			destruct_range_impl(mpl::make_index_sequence<sizeof...(Is)>{}, begin, end);
		}

		void destruct_at(ext::index index)
		{
			destruct_at_impl(mpl::make_index_sequence<sizeof...(Is)>{}, index);
		}

	private:
		template <std::size_t K>
		void construct_fill_zero_initialize_impl(mpl::true_type /*can_memset*/, mpl::index_sequence<K> seq, ext::index begin, ext::index end)
		{
			memset_fill_impl(seq, begin, end, ext::byte{});
		}

		template <std::size_t K>
		void construct_fill_zero_initialize_impl(mpl::false_type /*can_memset*/, mpl::index_sequence<K> seq, ext::index begin, ext::index end)
		{
			// todo forward initialization strategy
			construct_fill_impl(seq, begin, end);
		}

		template <std::size_t K, typename ...Ps>
		void construct_fill_impl(mpl::index_sequence<K> seq, ext::index begin, ext::index end, Ps && ...ps)
		{
			for (; begin != end; begin++)
			{
				construct_at_impl(seq, begin, ps...);
			}
		}

		template <std::size_t K>
		void construct_fill_impl(mpl::index_sequence<K> seq, ext::index begin, ext::index end, utility::zero_initialize_t)
		{
			construct_fill_zero_initialize_impl(can_memset_k<K>{}, seq, begin, end);
		}

		template <std::size_t ...Ks, typename ...Ps>
		void construct_fill_impl(mpl::index_sequence<Ks...>, ext::index begin, ext::index end, Ps && ...ps)
		{
			int expansion_hack[] = {(construct_fill_impl(mpl::index_sequence<Ks>{}, begin, end, ps...), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t ...Ks, typename ...Ps>
		void construct_fill_impl(mpl::index_sequence<Ks...>, ext::index begin, ext::index end, std::piecewise_construct_t, Ps && ...ps)
		{
			int expansion_hack[] = {(ext::apply([this, begin, end](auto && ...ps){ construct_fill_impl(mpl::index_sequence<Ks>{}, begin, end, std::forward<decltype(ps)>(ps)...); }, std::forward<Ps>(ps)), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t K, typename InputIt>
		void construct_range_or_memcpy_impl(mpl::true_type /*can_memcpy*/, mpl::index_sequence<K>, ext::index index, InputIt begin, InputIt end)
		{
			using utility::raw_range;
			auto range = raw_range(begin, end);

			assert(range.first <= range.second);
			std::memcpy(storage_->data(std::get<K>(it_)) + index, range.first, (range.second - range.first) * sizeof(value_type_k<K>));
		}

		template <std::size_t K, typename InputIt>
		void construct_range_or_memcpy_impl(mpl::false_type /*can_memcpy*/, mpl::index_sequence<K> seq, ext::index index, InputIt begin, InputIt end)
		{
			for (; begin != end; index++, ++begin)
			{
				construct_at_impl(seq, index, *begin);
			}
		}

		template <std::size_t K, typename InputIt>
		void construct_range_impl(mpl::index_sequence<K> seq, ext::index index, InputIt begin, InputIt end)
		{
			construct_range_or_memcpy_impl(can_memcpy_k<K, InputIt>{}, seq, index, begin, end);
		}

		template <std::size_t ...Ks, typename InputIt,
		          REQUIRES((sizeof...(Ks) != 1))>
		void construct_range_impl(mpl::index_sequence<Ks...>, ext::index index, InputIt begin, InputIt end)
		{
			int expansion_hack[] = {(construct_range_impl(mpl::index_sequence<Ks>{}, index, std::get<Ks>(begin), std::get<Ks>(end)), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t ...Ks, typename InputIt,
		          REQUIRES((sizeof...(Ks) != 1))>
		void construct_range_impl(mpl::index_sequence<Ks...>, ext::index index, std::move_iterator<InputIt> begin, std::move_iterator<InputIt> end)
		{
			int expansion_hack[] = {(construct_range_impl(mpl::index_sequence<Ks>{}, index, std::make_move_iterator(std::get<Ks>(begin.base())), std::make_move_iterator(std::get<Ks>(end.base()))), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t K, typename ...Ps>
		decltype(auto) construct_at_impl(mpl::index_sequence<K>, ext::index index, Ps && ...ps)
		{
			return storage_->construct_at(std::get<K>(it_) + index, std::forward<Ps>(ps)...);
		}

		template <std::size_t K, typename P>
		decltype(auto) construct_at_impl(mpl::index_sequence<K> seq, ext::index index, std::piecewise_construct_t, P && p)
		{
			return ext::apply([this, seq, index](auto && ...ps) -> decltype(auto) { return construct_at_impl(seq, index, std::forward<decltype(ps)>(ps)...); }, std::forward<P>(p));
		}

		template <std::size_t ...Ks, typename P,
		          REQUIRES((ext::tuple_size<P>::value == sizeof...(Is)))>
		decltype(auto) construct_at_impl(mpl::index_sequence<Ks...>, ext::index index, P && p)
		{
			return reference(construct_at_impl(mpl::index_sequence<Ks>{}, index, std::get<Ks>(std::forward<P>(p)))...);
		}

		template <std::size_t ...Ks, typename ...Ps>
		decltype(auto) construct_at_impl(mpl::index_sequence<Ks...>, ext::index index, Ps && ...ps)
		{
			return reference(construct_at_impl(mpl::index_sequence<Ks>{}, index, std::forward<Ps>(ps))...);
		}

		template <std::size_t ...Ks, typename ...Ps>
		decltype(auto) construct_at_impl(mpl::index_sequence<Ks...>, ext::index index, std::piecewise_construct_t, Ps && ...ps)
		{
			return reference(construct_at_impl(mpl::index_sequence<Ks>{}, index, std::piecewise_construct, std::forward<Ps>(ps))...);
		}

		template <std::size_t K, typename InputIt>
		void memcpy_range_impl(mpl::index_sequence<K> seq, ext::index index, InputIt begin, InputIt end)
		{
			construct_range_or_memcpy_impl(mpl::true_type{}, seq, index, begin, end);
		}

		template <std::size_t ...Ks, typename InputIt>
		void memcpy_range_impl(mpl::index_sequence<Ks...>, ext::index index, InputIt begin, InputIt end)
		{
			int expansion_hack[] = {(memcpy_range_impl(mpl::index_sequence<Ks>{}, index, std::get<Ks>(begin), std::get<Ks>(end)), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t K>
		void memset_fill_impl(mpl::index_sequence<K>, ext::index begin, ext::index end, ext::byte value)
		{
			std::memset(storage_->data(std::get<K>(it_)) + begin, static_cast<int>(value), (end - begin) * sizeof(value_type_k<K>));
		}

		template <std::size_t ...Ks>
		void memset_fill_impl(mpl::index_sequence<Ks...>, ext::index begin, ext::index end, ext::byte value)
		{
			int expansion_hack[] = {(memset_fill_impl(mpl::index_sequence<Ks>{}, begin, end, value), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t K>
		void destruct_range_impl(mpl::index_sequence<K> seq, ext::index begin, ext::index end)
		{
			for (; begin != end; begin++)
			{
				destruct_at_impl(seq, begin);
			}
		}

		template <std::size_t ...Ks>
		void destruct_range_impl(mpl::index_sequence<Ks...>, ext::index begin, ext::index end)
		{
			int expansion_hack[] = {(destruct_range_impl(mpl::index_sequence<Ks>{}, begin, end), 0)...};
			static_cast<void>(expansion_hack);
		}

		template <std::size_t K>
		void destruct_at_impl(mpl::index_sequence<K>, ext::index index)
		{
			storage_->destruct_at(std::get<K>(it_) + index);
		}

		template <std::size_t ...Ks>
		void destruct_at_impl(mpl::index_sequence<Ks...>, ext::index index)
		{
			int expansion_hack[] = {(destruct_at_impl(mpl::index_sequence<Ks>{}, index), 0)...};
			static_cast<void>(expansion_hack);
		}
	};

	template <typename Storage, std::size_t ...Is>
	class const_storage_data
	{
	private:
		using value_types = mpl::type_list<typename Storage::template value_type_at<Is>...>;
		using const_value_types = mpl::transform<std::add_const_t, value_types>;
		using storing_types = mpl::transform<Storage::template storing_type_for, value_types>;
		using const_storing_types = mpl::transform<std::add_const_t, storing_types>;

		using const_pointers = mpl::transform<std::add_pointer_t, const_value_types>;
		using const_references = mpl::transform<std::add_lvalue_reference_t, const_value_types>;

		using const_iterator = mpl::apply<utility::zip_iterator,
		                                  mpl::transform<std::add_pointer_t,
		                                                 const_storing_types>>;

	public:
		using value_type = utility::combine<utility::compound, value_types>;
		using const_reference = utility::combine<utility::compound, const_references>;
		using const_pointer = utility::combine<utility::zip_pointer, const_pointers>;

	private:
		const Storage * storage_;
		const_iterator it_;

	public:
		template <typename ...Ps>
		explicit const_storage_data(const Storage & storage, Ps && ...ps)
			: storage_(&storage)
			, it_(std::forward<Ps>(ps)...)
		{}

	public:
		const_reference operator [] (ext::index index) const
		{
			return ext::apply([&](auto & ...ps) -> const_reference { return const_reference(*storage_->data(ps + index)...); }, it_);
		}

		const_pointer data() const
		{
			return ext::apply([&](auto & ...ps){ return const_pointer(storage_->data(ps)...); }, it_);
		}
	};

	template <std::size_t Capacity, typename ...Ts>
	class static_storage
	{
		using this_type = static_storage<Capacity, Ts...>;

	public:
		using value_type = utility::combine<utility::compound, Ts...>;
		using allocator_type = utility::null_allocator<char>;
		using size_type = std::size_t; // todo size_for<Capacity>?
		using difference_type = std::ptrdiff_t;
		using reference = utility::combine<utility::compound, Ts &...>;
		using const_reference = utility::combine<utility::compound, const Ts &...>;
		using rvalue_reference = utility::combine<utility::compound, Ts &&...>;
		using const_rvalue_reference = utility::combine<utility::compound, const Ts &&...>;
		using pointer = utility::combine<utility::zip_pointer, Ts *...>;
		using const_pointer = utility::combine<utility::zip_pointer, const Ts *...>;

		template <std::size_t I>
		using value_type_at = mpl::type_at<I, Ts...>; // todo remove
		template <typename Storing>
		using value_type_for = typename Storing::value_type;
		template <typename T>
		using storing_type_for = utility::storing<T>;

		using data_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<utility::storing<Ts>>...>;

		using storing_trivially_copyable =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_copyable,
			                          utility::storing<Ts>...>>;
		using storing_trivially_destructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_destructible,
			                          utility::storing<Ts>...>>;
		using storing_trivially_default_constructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_default_constructible,
			                          utility::storing<Ts>...>>;

	private:
		utility::tuple<std::array<storing_type_for<Ts>, Capacity>...> arrays;

	public:
		bool allocate(std::size_t capacity)
		{
			return /*debug_assert*/(capacity == Capacity);
		}

		constexpr bool good() const
		{
			return true;
		}

		void deallocate(std::size_t capacity)
		{
			assert(capacity == Capacity);
			static_cast<void>(capacity);
		}

		constexpr std::size_t max_size() const { return Capacity; }

		template <typename T, typename ...Ps>
		T & construct_at(utility::storing<T> * ptr_, Ps && ...ps)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return ptr_->construct(std::forward<Ps>(ps)...);
		}

		template <typename T>
		void destruct_at(utility::storing<T> * ptr_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			ptr_->destruct();
		}

		template <typename T>
		T * data(utility::storing<T> * data_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return &data_->value;
		}
		template <typename T>
		const T * data(const utility::storing<T> * data_) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return &data_->value;
		}

		template <std::size_t ...Is>
		auto sections_for(std::size_t /*capacity*/, mpl::index_sequence<Is...>)
		{
			return utility::storage_data<this_type, Is...>(*this, get<Is>(arrays).data()...);
		}

		template <std::size_t ...Is>
		auto sections_for(std::size_t /*capacity*/, mpl::index_sequence<Is...>) const
		{
			return utility::const_storage_data<this_type, Is...>(*this, get<Is>(arrays).data()...);
		}

		auto sections(std::size_t capacity)
		{
			return sections_for(capacity, mpl::make_index_sequence_for<Ts...>{});
		}

		auto sections(std::size_t capacity) const
		{
			return sections_for(capacity, mpl::make_index_sequence_for<Ts...>{});
		}
	};

	template <template <typename> class Allocator, typename ...Ts>
	class dynamic_storage
	{
		using this_type = dynamic_storage<Allocator, Ts...>;

	public:
		using value_type = utility::combine<utility::compound, Ts...>;
		using allocator_type = utility::aggregation_allocator<Allocator, void, Ts...>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = utility::combine<utility::compound, Ts &...>;
		using const_reference = utility::combine<utility::compound, const Ts &...>;
		using rvalue_reference = utility::combine<utility::compound, Ts &&...>;
		using const_rvalue_reference = utility::combine<utility::compound, const Ts &&...>;
		using pointer = utility::combine<utility::zip_pointer, Ts *...>;
		using const_pointer = utility::combine<utility::zip_pointer, const Ts *...>;

		template <std::size_t I>
		using value_type_at = mpl::type_at<I, Ts...>; // todo remove
		template <typename Storing>
		using value_type_for = Storing;
		template <typename T>
		using storing_type_for = T;

		using data_trivially_copyable = mpl::conjunction<std::is_trivially_copyable<Ts>...>;

		using storing_trivially_copyable =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_copyable,
			                          Ts...>>;
		using storing_trivially_destructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_destructible,
			                          Ts...>>;
		using storing_trivially_default_constructible =
			mpl::apply<mpl::conjunction,
			           mpl::transform<std::is_trivially_default_constructible,
			                          Ts...>>;

	private:
		using allocator_traits = std::allocator_traits<allocator_type>;

	private:
		struct empty_allocator_hack : allocator_type
		{
#if MODE_DEBUG
			void * storage_ = nullptr;

			~empty_allocator_hack()
			{
				assert(!storage_);
			}
			empty_allocator_hack() = default;
			empty_allocator_hack(empty_allocator_hack && other)
				: storage_(std::exchange(other.storage_, nullptr))
			{}
			empty_allocator_hack & operator = (empty_allocator_hack && other)
			{
				assert(!storage_);

				storage_ = std::exchange(other.storage_, nullptr);

				return *this;
			}
#else
			void * storage_;

			empty_allocator_hack() = default;
			empty_allocator_hack(empty_allocator_hack && other) = default;
			empty_allocator_hack & operator = (empty_allocator_hack && other) = default;
#endif
		} impl_;

	public:
		bool allocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(!storage());
#endif
			storage() = allocator_traits::allocate(allocator(), capacity);
			return good();
		}

		bool good() const
		{
			return storage() != nullptr;
		}

		void deallocate(std::size_t capacity)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::deallocate(allocator(), storage(), capacity);
#if MODE_DEBUG
			storage() = nullptr;
#endif
		}

		template <typename T, typename ...Ps>
		T & construct_at(T * ptr_, Ps && ...ps)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::construct(allocator(), ptr_, std::forward<Ps>(ps)...);
			return *ptr_;
		}

		template <typename T>
		void destruct_at(T * ptr_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::destroy(allocator(), ptr_);
		}

		template <typename T>
		T * data(T * data_)
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return data_;
		}
		template <typename T>
		const T * data(const T * data_) const
		{
			static_assert(mpl::member_of<T, Ts...>::value, "");
			return data_;
		}

		template <std::size_t ...Is>
		auto sections_for(std::size_t capacity, mpl::index_sequence<Is...>)
		{
#if MODE_DEBUG
			assert(storage() || capacity == 0);
#endif
			// note there are times when we will call allocator
			// address with a garbage storage, but maybe that is okay
			// since with a capacity of zero we should never actually
			// use the address?
			return utility::storage_data<this_type, Is...>(*this, allocator().template address<Is>(storage(), capacity)...);
		}

		template <std::size_t ...Is>
		auto sections_for(std::size_t capacity, mpl::index_sequence<Is...>) const
		{
#if MODE_DEBUG
			assert(storage() || capacity == 0);
#endif
			// note there are times when we will call allocator
			// address with a garbage storage, but maybe that is okay
			// since with a capacity of zero we should never actually
			// use the address?
			return utility::const_storage_data<this_type, Is...>(*this, allocator().template address<Is>(storage(), capacity)...);
		}

		auto sections(std::size_t capacity)
		{
			return sections_for(capacity, mpl::make_index_sequence_for<Ts...>{});
		}

		auto sections(std::size_t capacity) const
		{
			return sections_for(capacity, mpl::make_index_sequence_for<Ts...>{});
		}

	private:
		allocator_type & allocator() { return impl_; }
		const allocator_type & allocator() const { return impl_; }

		void * & storage() { return impl_.storage_; }
		const void * storage() const { return impl_.storage_; }
	};

	template <typename ...Ts>
	using heap_storage = dynamic_storage<utility::heap_allocator, Ts...>;

	template <typename Storage>
	struct storage_traits;
	template <template <typename> class Allocator, typename ...Ts>
	struct storage_traits<dynamic_storage<Allocator, Ts...>>
	{
		template <typename T>
		using allocator_type = Allocator<T>;
		using type_list = mpl::type_list<Ts...>; // todo remove, and implement storage_element instead

		template <typename ...Us>
		using storage_type = dynamic_storage<Allocator, Us...>;
		template <typename ...Us>
		using append = dynamic_storage<Allocator, Ts..., Us...>; // todo remove

		using static_capacity = mpl::false_type;
		using trivial_allocate = mpl::false_type;
		using trivial_deallocate = mpl::false_type;
		using moves_allocation = mpl::true_type;

		static constexpr std::size_t size = sizeof...(Ts); // todo remove
	};
	template <std::size_t Capacity, typename ...Ts>
	struct storage_traits<static_storage<Capacity, Ts...>>
	{
		template <typename T>
		using allocator_type = utility::null_allocator<T>;
		using type_list = mpl::type_list<Ts...>; // todo remove, and implement storage_element instead

		template <typename ...Us>
		using storage_type = static_storage<Capacity, Us...>;
		template <typename ...Us>
		using append = static_storage<Capacity, Ts..., Us...>; // todo remove

		using static_capacity = mpl::true_type;
		using trivial_allocate = mpl::true_type;
		using trivial_deallocate = mpl::true_type;
		using moves_allocation = mpl::false_type;

		static constexpr std::size_t size = sizeof...(Ts); // todo remove
		static constexpr std::size_t capacity_value = Capacity;
	};

	template <typename Storage>
	using storage_size = mpl::index_constant<storage_traits<Storage>::size>;

	template <std::size_t Capacity>
	using static_storage_traits = storage_traits<static_storage<Capacity, int>>; // todo remove int?
	template <template <typename> class Allocator>
	using dynamic_storage_traits = storage_traits<dynamic_storage<Allocator, int>>; // todo remove int?
	using heap_storage_traits = dynamic_storage_traits<heap_allocator>;
	using null_storage_traits = dynamic_storage_traits<null_allocator>;

	template <typename Storage>
	using storage_is_trivially_destructible =
		mpl::conjunction<typename Storage::storing_trivially_destructible,
		                 typename storage_traits<Storage>::trivial_deallocate>;
	template <typename Storage>
	using storage_is_trivially_default_constructible =
		mpl::conjunction<typename Storage::storing_trivially_default_constructible,
		                 typename storage_traits<Storage>::trivial_allocate>;

	template <typename Storage>
	struct reserve_exact
	{
		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t /*size*/)
		{
			return StorageTraits::capacity_value;
		}

		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((!StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t size)
		{
			return size;
		}
	};

	template <typename Storage>
	struct reserve_power_of_two
	{
		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((StorageTraits::static_capacity::value))>
		constexpr std::size_t operator () (std::size_t /*size*/)
		{
			return StorageTraits::capacity_value;
		}

		template <typename StorageTraits = storage_traits<Storage>,
		          REQUIRES((!StorageTraits::static_capacity::value))>
		/*constexpr*/ std::size_t operator () (std::size_t size)
		{
			return utility::clp2(size);
		}
	};

	template <template <typename> class ReservationStrategy>
	struct reserve_nonempty
	{
		template <typename Storage>
		struct type
		{
			constexpr std::size_t operator () (std::size_t size)
			{
				return ReservationStrategy<Storage>{}(size == 0 ? 1 : size);
			}
		};
	};
}
