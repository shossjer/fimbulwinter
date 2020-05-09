
#ifndef UTILITY_STORAGE_HPP
#define UTILITY_STORAGE_HPP

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

	template <typename Storage, bool Const, std::size_t ...Is>
	class storage_iterator_impl
	{
		static_assert(0 < sizeof...(Is), "");

		template <typename Storage_, bool Const_, std::size_t ...Is_>
		friend class storage_iterator_impl;

		using this_type = storage_iterator_impl<Storage, Const, Is...>;

		using value_types = mpl::type_list<typename Storage::template value_type_at<Is>...>;
		using storing_types = mpl::transform<Storage::template storing_type_for, value_types>;

		template <typename T>
		using constify = mpl::add_const_if<Const, T>;

		using constified_storage_type = mpl::add_const_if<Const, Storage>;
		using constified_value_types = mpl::transform<constify, value_types>;
		using constified_storing_types = mpl::transform<constify, storing_types>;

		using pointers = mpl::transform<std::add_pointer_t, constified_value_types>;
		using references = mpl::transform<std::add_lvalue_reference_t, constified_value_types>;
		using rvalue_references = mpl::transform<std::add_rvalue_reference_t, constified_value_types>;
		using iterator = mpl::apply<utility::zip_iterator,
		                            mpl::transform<std::add_pointer_t,
		                                           constified_storing_types>>;

	public:
		using difference_type = typename iterator::difference_type;
		using value_type = utility::compound_type<value_types>;
		using pointer = utility::zip_type<utility::zip_iterator, pointers>;
		using reference = utility::compound_type<references>;
		using rvalue_reference = utility::compound_type<rvalue_references>;
		using iterator_category = std::random_access_iterator_tag; // ??

	private:
		constified_storage_type * storage_;
		iterator ptr_;

	public:
		template <typename ...Ps,
		          REQUIRES((std::is_constructible<iterator, Ps...>::value))>
		explicit storage_iterator_impl(constified_storage_type & storage_, Ps && ...ps)
			: storage_(&storage_)
			, ptr_(std::forward<Ps>(ps)...)
		{}

	public:
		constified_storage_type & storage() { return *storage_; }
		const Storage & storage() const { return *storage_; }

		iterator & base() { return ptr_; }
		const iterator & base() const { return ptr_; }

		reference operator * () const
		{
			return ext::apply([this](auto & ...ps) -> reference { return reference(storage_->value_at(ps)...); }, ptr_);
		}

		reference operator [] (difference_type n) const
		{
			return ext::apply([this, n](auto & ...ps) -> reference { return reference(storage_->value_at(ps + n)...); }, ptr_);
		}

		this_type & operator ++ () { ++ptr_; return *this; }
		this_type & operator -- () { --ptr_; return *this; }
		this_type operator ++ (int) { return this_type(*storage_, ptr_++); }
		this_type operator -- (int) { return this_type(*storage_, ptr_--); }
		this_type operator + (difference_type n) { return this_type(*storage_, ptr_ + n); }
		this_type operator - (difference_type n) { return this_type(*storage_, ptr_ - n); }
		this_type & operator += (difference_type n) { ptr_ += n; return *this; }
		this_type & operator -= (difference_type n) { ptr_ -= n; return *this; }

		friend this_type operator + (difference_type n, const this_type & x) { return x + n; }

	private:
		friend rvalue_reference iter_move(this_type x)
		{
#if defined(_MSC_VER) && _MSC_VER <= 1916
			using rvalue_reference = rvalue_reference;
#endif
			return ext::apply([&x](auto & ...ps) -> rvalue_reference { return rvalue_reference(std::move(x.storage_->value_at(ps))...); }, x.ptr_);
		}

		friend pointer to_address(this_type x)
		{
			return ext::apply([&](auto & ...ps){ return pointer(x.storage_->data(ps)...); }, x.ptr_);
		}
	};

	template <typename Storage, std::size_t ...Is>
	using storage_iterator = storage_iterator_impl<Storage, false, Is...>;
	template <typename Storage, std::size_t ...Is>
	using const_storage_iterator = storage_iterator_impl<Storage, true, Is...>;

	template <typename Storage, std::size_t ...Is>
	class storage_data
	{
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

		using iterator = storage_iterator<Storage, Is...>;
		using const_iterator = const_storage_iterator<Storage, Is...>;

	public:
		using value_type = typename iterator::value_type;
		using reference = typename iterator::reference;
		using const_reference = typename const_iterator::reference;
		using pointer = typename iterator::pointer;
		using const_pointer = typename const_iterator::pointer;

		template <typename InputIt>
		using can_memcpy = mpl::conjunction<std::is_trivially_copyable<storing_type_i<Is>>...,
		                                    utility::is_contiguous_iterator<InputIt>>;

		using can_memset = mpl::conjunction<std::is_trivially_copyable<storing_type_i<Is>>...>;

	private:
		iterator it_;

	public:
		template <typename ...Ps>
		explicit storage_data(Storage & storage, Ps && ...ps)
			: it_(storage, std::forward<Ps>(ps)...)
		{}

	public:
		reference operator [] (ext::index index)
		{
			return it_[index];
		}

		const_reference operator [] (ext::index index) const
		{
			return it_[index];
		}

		pointer data() { return to_address(it_); }
		const_pointer data() const { return to_address(it_); }

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
			std::memcpy(it_.storage().data(std::get<K>(it_.base())) + index, range.first, (range.second - range.first) * sizeof(value_type_k<K>));
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
			return it_.storage().construct_at(std::get<K>(it_.base()), index, std::forward<Ps>(ps)...);
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
			std::memset(it_.storage().data(std::get<K>(it_.base())) + begin, static_cast<int>(value), (end - begin) * sizeof(value_type_k<K>));
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
			it_.storage().destruct_at(std::get<K>(it_.base()), index);
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
		using iterator = const_storage_iterator<Storage, Is...>;
		using const_iterator = iterator;

	public:
		using value_type = typename iterator::value_type;
		using reference = typename iterator::reference;
		using const_reference = typename const_iterator::reference;
		using pointer = typename iterator::pointer;
		using const_pointer = typename const_iterator::pointer;

	private:
		iterator it_;

	public:
		template <typename ...Ps>
		explicit const_storage_data(const Storage & storage, Ps && ...ps)
			: it_(storage, std::forward<Ps>(ps)...)
		{}

	public:
		const_reference operator [] (ext::index index) const
		{
			return it_[index];
		}

		const_pointer data() const { return to_address(it_); }
	};

	template <std::size_t Capacity, typename ...Ts>
	class static_storage
	{
		using this_type = static_storage<Capacity, Ts...>;

	public:
		using value_type = utility::compound_type<Ts...>;
		using allocator_type = utility::null_allocator<char>;
		using size_type = std::size_t; // todo size_for<Capacity>?
		using difference_type = std::ptrdiff_t;
		using reference = utility::compound_type<Ts &...>;
		using const_reference = utility::compound_type<const Ts &...>;
		using rvalue_reference = utility::compound_type<Ts &&...>;
		using pointer = utility::zip_type<utility::zip_iterator, Ts *...>;
		using const_pointer = utility::zip_type<utility::zip_iterator, const Ts *...>;

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

		template <typename StoringType, typename ...Ps>
		typename StoringType::value_type & construct_at(StoringType * data_, std::ptrdiff_t index, Ps && ...ps)
		{
			assert(std::size_t(index) < Capacity);
			return data_[index].construct(std::forward<Ps>(ps)...);
		}

		template <typename StoringType>
		void destruct_at(StoringType * data_, std::ptrdiff_t index)
		{
			assert(std::size_t(index) < Capacity);
			data_[index].destruct();
		}

		template <typename StoringType>
		typename StoringType::value_type * data(StoringType * data_)
		{
			return &data_->value;
		}
		template <typename StoringType>
		const typename StoringType::value_type * data(const StoringType * data_) const
		{
			return &data_->value;
		}

		template <typename StoringType>
		std::ptrdiff_t index_of(const StoringType * data_, const typename StoringType::value_type & x) const
		{
			// x is pointer interconvertible with storing_t, since
			// storing_t is a union containing a value_type member
			return reinterpret_cast<const StoringType *>(std::addressof(x)) - data_;
		}

		template <typename StoringType>
		typename StoringType::value_type & value_at(StoringType * p)
		{
			return p->value;
		}
		template <typename StoringType>
		const typename StoringType::value_type & value_at(const StoringType * p) const
		{
			return p->value;
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
		using value_type = utility::compound_type<Ts...>;
		using allocator_type = utility::aggregation_allocator<Allocator, void, Ts...>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using reference = utility::compound_type<Ts &...>;
		using const_reference = utility::compound_type<const Ts &...>;
		using rvalue_reference = utility::compound_type<Ts &&...>;
		using pointer = utility::zip_type<utility::zip_iterator, Ts *...>;
		using const_pointer = utility::zip_type<utility::zip_iterator, const Ts *...>;

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
		T & construct_at(T * data_, std::ptrdiff_t index, Ps && ...ps)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::construct(allocator(), data_ + index, std::forward<Ps>(ps)...);
			return data_[index];
		}

		template <typename T>
		void destruct_at(T * data_, std::ptrdiff_t index)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			allocator_traits::destroy(allocator(), data_ + index);
		}

		template <typename T>
		T * data(T * data_)
		{
			return data_;
		}
		template <typename T>
		const T * data(const T * data_) const
		{
			return data_;
		}

		template <typename T>
		std::ptrdiff_t index_of(const T * data_, const T & x) const
		{
#if MODE_DEBUG
			assert(storage());
#endif
			return std::addressof(x) - data_;
		}

		template <typename T>
		T & value_at(T * p)
		{
#if MODE_DEBUG
			assert(storage());
#endif
			return *p;
		}
		template <typename T>
		const T & value_at(const T * p) const
		{
#if MODE_DEBUG
			assert(storage());
#endif
			return *p;
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

#endif /* UTILITY_STORAGE_HPP */
