
#ifndef UTILITY_STORAGE_HPP
#define UTILITY_STORAGE_HPP

#include "config.h"

#include "utility/algorithm.hpp"
#include "utility/aggregation_allocator.hpp"
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
	template <typename Storage, typename T>
	class section_iterator
	{
	private:
		using this_type = section_iterator<Storage, T>;

		using storing_type = typename Storage::template storing_type_for<T>;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = T *;
		using reference = T &;
		using iterator_category = std::random_access_iterator_tag;

		using rvalue_reference = T &&;

	private:
		Storage * storage_;
		storing_type * ptr_;

	public:
		section_iterator(Storage & storage_, storing_type * ptr_)
			: storage_(&storage_)
			, ptr_(ptr_)
		{}

	public:
		Storage & storage() { return *storage_; }
		const Storage & storage() const { return *storage_; }

		storing_type * base() { return ptr_; }
		const storing_type * base() const { return ptr_; }

		reference operator * () const
		{
			return storage_->value_at(ptr_);
		}

		reference operator [] (difference_type n) const
		{
			return storage_->value_at(ptr_ + n);
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
		friend std::pair<T *, T *> raw_range(this_type begin, this_type end)
		{
			return std::make_pair(begin.storage_->data(begin.ptr_), end.storage_->data(end.ptr_));
		}
	};

	template <typename Storage, typename T>
	bool operator == (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() == i2.base();
	}
	template <typename Storage, typename T>
	bool operator != (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() != i2.base();
	}
	template <typename Storage, typename T>
	bool operator < (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() < i2.base();
	}
	template <typename Storage, typename T>
	bool operator <= (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() <= i2.base();
	}
	template <typename Storage, typename T>
	bool operator > (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() > i2.base();
	}
	template <typename Storage, typename T>
	bool operator >= (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() >= i2.base();
	}

	template <typename Storage, typename T>
	auto operator - (const section_iterator<Storage, T> & i1, const section_iterator<Storage, T> & i2)
	{
		return i1.base() - i2.base();
	}

	template <typename Storage, typename T>
	class const_section_iterator
	{
	private:
		using this_type = const_section_iterator<Storage, T>;

		using storing_type = typename Storage::template storing_type_for<T>;

	public:
		using difference_type = std::ptrdiff_t;
		using value_type = T;
		using pointer = const T *;
		using reference = const T &;
		using iterator_category = std::random_access_iterator_tag;

		using rvalue_reference = const T &&;

	private:
		const Storage * storage_;
		const storing_type * ptr_;

	public:
		const_section_iterator(const Storage & storage_, const storing_type * ptr_)
			: storage_(&storage_)
			, ptr_(ptr_)
		{}

	public:
		const Storage & storage() const { return *storage_; }

		const storing_type * base() const { return ptr_; }

		reference operator * () const
		{
			return storage_->value_at(ptr_);
		}

		reference operator [] (difference_type n) const
		{
			return storage_->value_at(ptr_ + n);
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
		friend std::pair<const T *, const T *> raw_range(this_type begin, this_type end)
		{
			return std::make_pair(begin.storage_->data(begin.ptr_), end.storage_->data(end.ptr_));
		}
	};

	template <typename Storage, typename T>
	bool operator == (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() == i2.base();
	}
	template <typename Storage, typename T>
	bool operator != (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() != i2.base();
	}
	template <typename Storage, typename T>
	bool operator < (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() < i2.base();
	}
	template <typename Storage, typename T>
	bool operator <= (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() <= i2.base();
	}
	template <typename Storage, typename T>
	bool operator > (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() > i2.base();
	}
	template <typename Storage, typename T>
	bool operator >= (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() >= i2.base();
	}

	template <typename Storage, typename T>
	auto operator - (const const_section_iterator<Storage, T> & i1, const const_section_iterator<Storage, T> & i2)
	{
		return i1.base() - i2.base();
	}

	template <typename Storage, typename T>
	class section
	{
	public:
		using iterator = section_iterator<Storage, T>;
		using const_iterator = const_section_iterator<Storage, T>;

		using value_type = T;
		using reference = typename iterator::reference;
		using pointer = typename iterator::pointer;
		using const_reference = typename const_iterator::reference;
		using const_pointer = typename const_iterator::pointer;

		using storing_type = typename Storage::template storing_type_for<value_type>;
		using storing_trivially_copyable = std::is_trivially_copyable<storing_type>;
		using storing_trivially_destructible = std::is_trivially_destructible<storing_type>;

		template <typename InputIt>
		using can_memcpy = mpl::conjunction<storing_trivially_copyable,
		                                    utility::is_contiguous_iterator<InputIt>>;

	private:
		iterator ptr_;

	public:
		section(iterator && ptr_)
			: ptr_(std::move(ptr_))
		{}

	public:
		reference operator [] (std::ptrdiff_t index)
		{
			return ptr_[index];
		}
		const_reference operator [] (std::ptrdiff_t index) const
		{
			return ptr_[index];
		}

		template <typename ...Ps>
		void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			for (; begin != end; begin++)
			{
				ptr_.storage().construct_at(ptr_.base(), begin, ps...);
			}
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(can_memcpy<InputIt>{}, index, begin, end);
		}

		template <typename ...Ps>
		reference construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			return ptr_.storage().construct_at(ptr_.base(), index, std::forward<Ps>(ps)...);
		}

		template <typename InputIt,
		          REQUIRES((can_memcpy<InputIt>::value))>
		void memcpy_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(mpl::true_type{}, index, begin, end);
		}

		pointer data()
		{
			return ptr_.storage().data(ptr_.base());
		}
		const_pointer data() const
		{
			return ptr_.storage().data(ptr_.base());
		}

		void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			for (; begin != end; begin++)
			{
				ptr_.storage().destruct_at(ptr_.base(), begin);
			}
		}

		void destruct_at(std::ptrdiff_t index)
		{
			ptr_.storage().destruct_at(ptr_.base(), index);
		}

		std::ptrdiff_t index_of(const_reference x) const
		{
			return ptr_.storage().index_of(ptr_.base(), x);
		}
	private:
		template <typename InputIt>
		void construct_range_impl(mpl::true_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			using utility::raw_range;
			auto range = raw_range(begin, end);

			assert(range.first <= range.second);
			std::memcpy(ptr_.storage().data(ptr_.base()) + index, range.first, (range.second - range.first) * sizeof(value_type));
		}

		template <typename InputIt>
		void construct_range_impl(mpl::false_type /*can_memcpy*/, std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			for (; begin != end; index++, ++begin)
			{
				ptr_.storage().construct_at(ptr_.base(), index, *begin);
			}
		}
	};

	template <typename Storage, typename T>
	class const_section
	{
	public:
		using const_iterator = const_section_iterator<Storage, T>;

		using value_type = T;
		using const_reference = typename const_iterator::reference;
		using const_pointer = typename const_iterator::pointer;

		using storing_type = typename Storage::template storing_type_for<value_type>;

	private:
		const_iterator ptr_;

	public:
		const_section(const_iterator && ptr_)
			: ptr_(std::move(ptr_))
		{}

	public:
		const_reference operator [] (std::ptrdiff_t index) const
		{
			return ptr_[index];
		}

		const_pointer data() const
		{
			return ptr_.storage().data(ptr_.base());
		}

		std::ptrdiff_t index_of(const_reference x) const
		{
			return ptr_.storage().index_of(ptr_.base(), x);
		}
	};

	template <typename Storage>
	class storage_iterator
	{
	private:
		using this_type = storage_iterator<Storage>;

		using value_types = typename Storage::value_types;
		using storing_types = mpl::transform<Storage::template storing_type_for, value_types>;
		using references = mpl::transform<std::add_lvalue_reference_t, value_types>;
		using rvalue_references = mpl::transform<std::add_rvalue_reference_t, value_types>;
		using iterator = mpl::apply<utility::zip_iterator,
		                            mpl::transform<std::add_pointer_t,
		                                           storing_types>>;

	public:
		using value_type = mpl::apply<std::tuple, value_types>;
		using difference_type = typename iterator::difference_type;
		using reference = mpl::apply<utility::proxy_reference, references>;
		using rvalue_reference = mpl::apply<utility::proxy_reference, rvalue_references>;

		using pointer = void *; // ??
		using iterator_category = std::input_iterator_tag; // ??

	private:
		Storage * storage_;
		iterator ptr_;

	public:
		template <typename ...Ps,
		          REQUIRES((std::is_constructible<iterator, Ps...>::value))>
		storage_iterator(Storage & storage_, Ps && ...ps)
			: storage_(&storage_)
			, ptr_(std::forward<Ps>(ps)...)
		{}

	public:
		Storage & storage() { return *storage_; }
		const Storage & storage() const { return *storage_; }

		iterator & base() { return ptr_; }
		const iterator & base() const { return ptr_; }

		reference operator * () const
		{
			return utl::unpack(tuple(), [this](auto & ...ps){ return reference(storage_->value_at(ps)...); });
		}

		reference operator [] (difference_type n) const
		{
			return utl::unpack(tuple(), [this, n](auto & ...ps){ return reference(storage_->value_at(ps + n)...); });
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
		typename iterator::underlying_type & tuple() { return ptr_; }
		const typename iterator::underlying_type & tuple() const { return ptr_; }

	private:
		friend rvalue_reference iter_move(this_type x)
		{
			return utl::unpack(x.tuple(), [&x](auto & ...ps){ return rvalue_reference(std::move(x.storage_->value_at(ps))...); });
		}
	};

	template <std::size_t I, typename Storage>
	decltype(auto) get(storage_iterator<Storage> & that) { return section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(that.base())); }
	template <std::size_t I, typename Storage>
	decltype(auto) get(const storage_iterator<Storage> & that) { return section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(that.base())); }
	template <std::size_t I, typename Storage>
	decltype(auto) get(storage_iterator<Storage> && that) { return section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(std::move(that.base()))); }
	template <std::size_t I, typename Storage>
	decltype(auto) get(const storage_iterator<Storage> && that) { return section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(std::move(that.base()))); }
	template <typename T, typename Storage>
	decltype(auto) get(storage_iterator<Storage> & that) { return get<mpl::index_of<T, typename Storage::value_types>>(that); }
	template <typename T, typename Storage>
	decltype(auto) get(const storage_iterator<Storage> & that) { return get<mpl::index_of<T, typename Storage::value_types>>(that); }
	template <typename T, typename Storage>
	decltype(auto) get(storage_iterator<Storage> && that) { return get<mpl::index_of<T, typename Storage::value_types>>(std::move(that)); }
	template <typename T, typename Storage>
	decltype(auto) get(const storage_iterator<Storage> && that) { return get<mpl::index_of<T, typename Storage::value_types>>(std::move(that)); }

	template <typename Storage>
	class const_storage_iterator
	{
	private:
		using this_type = const_storage_iterator<Storage>;

		using value_types = typename Storage::value_types;
		using storing_types = mpl::transform<Storage::template storing_type_for, value_types>;
		using const_references = mpl::transform<std::add_lvalue_reference_t,
		                                        mpl::transform<std::add_const_t,
		                                                       value_types>>;
		using const_iterator = mpl::apply<utility::zip_iterator,
		                                  mpl::transform<std::add_pointer_t,
		                                                 mpl::transform<std::add_const_t,
		                                                                storing_types>>>;

	public:
		using value_type = mpl::apply<std::tuple, value_types>;
		using difference_type = typename const_iterator::difference_type;
		using reference = mpl::apply<utility::proxy_reference, const_references>;

	private:
		const Storage * storage_;
		const_iterator ptr_;

	public:
		template <typename ...Ps,
		          REQUIRES((std::is_constructible<const_iterator, Ps...>::value))>
		const_storage_iterator(const Storage & storage_, Ps && ...ps)
			: storage_(&storage_)
			, ptr_(std::forward<Ps>(ps)...)
		{}

	public:
		const Storage & storage() const { return *storage_; }

		const const_iterator & base() const { return ptr_; }

		reference operator * () const
		{
			return utl::unpack(tuple(), [this](auto & ...ps){ return reference(storage_->value_at(ps)...); });
		}

		reference operator [] (difference_type n) const
		{
			return utl::unpack(tuple(), [this, n](auto & ...ps){ return reference(storage_->value_at(ps + n)...); });
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
		typename const_iterator::underlying_type & tuple() { return ptr_; }
		const typename const_iterator::underlying_type & tuple() const { return ptr_; }

	private:
		// todo const_rvalue_reference?
		friend auto iter_move(this_type x)
		{
			return utl::unpack(x.tuple(), [&x](auto & ...ps){ return utility::make_proxy_reference(std::move(x.storage_->value_at(ps))...); });
		}
	};

	template <std::size_t I, typename Storage>
	decltype(auto) get(const const_storage_iterator<Storage> & that) { return const_section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(that.base())); }
	template <std::size_t I, typename Storage>
	decltype(auto) get(const const_storage_iterator<Storage> && that) { return const_section_iterator<Storage, typename Storage::template value_type_at<I>>(that.storage(), utility::get<I>(std::move(that.base()))); }
	template <typename T, typename Storage>
	decltype(auto) get(const const_storage_iterator<Storage> & that) { return get<mpl::index_of<T, typename Storage::value_types>>(that); }
	template <typename T, typename Storage>
	decltype(auto) get(const const_storage_iterator<Storage> && that) { return get<mpl::index_of<T, typename Storage::value_types>>(std::move(that)); }

	template <typename Storage>
	class storage_data
	{
	private:
		using value_types = typename Storage::value_types;

	public:
		using iterator = storage_iterator<Storage>;
		using const_iterator = const_storage_iterator<Storage>;

		using reference = typename iterator::reference;
		using const_reference = typename const_iterator::reference;

	private:
		iterator ptr_;

	public:
		storage_data(iterator && ptr)
			: ptr_(std::move(ptr))
		{}

	public:
		reference operator [] (std::ptrdiff_t index)
		{
			return ptr_[index];
		}
		const_reference operator [] (std::ptrdiff_t index) const
		{
			return ptr_[index];
		}

		iterator data() { return ptr_; }
		const_iterator data() const { return ptr_; }

		template <typename ...Ps>
		void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			return construct_fill_impl(mpl::make_index_sequence<value_types::size>{}, begin, end, std::forward<Ps>(ps)...);
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			construct_range_impl(mpl::make_index_sequence<value_types::size>{}, index, begin, end);
		}

		template <typename ...Ps>
		reference construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			return construct_at_impl(mpl::make_index_sequence<value_types::size>{}, index, std::forward<Ps>(ps)...);
		}
		template <typename ...Ps>
		reference construct_at(std::ptrdiff_t index, std::piecewise_construct_t, Ps && ...ps)
		{
			return piecewise_construct_at_impl(mpl::make_index_sequence<value_types::size>{}, index, std::forward<Ps>(ps)...);
		}

		// todo memcpy_range

		void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			destruct_range_impl(mpl::make_index_sequence<value_types::size>{}, begin, end);
		}

		void destruct_at(std::ptrdiff_t index)
		{
			destruct_at_impl(mpl::make_index_sequence<value_types::size>{}, index);
		}

		template <std::size_t I,
		          REQUIRES((I < value_types::size))>
		utility::section<Storage, typename Storage::template value_type_at<I>> section(mpl::index_constant<I>)
		{
			return utility::section<Storage, typename Storage::template value_type_at<I>>(utility::get<I>(ptr_));
		}
		template <std::size_t I,
		          REQUIRES((I < value_types::size))>
		utility::const_section<Storage, typename Storage::template value_type_at<I>> section(mpl::index_constant<I>) const
		{
			return utility::const_section<Storage, typename Storage::template value_type_at<I>>(utility::get<I>(ptr_));
		}
	private:
		template <std::size_t ...Is, typename ...Ps>
		void construct_fill_impl(mpl::index_sequence<Is...>, std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			int expansion_hack[] = {(section(mpl::index_constant<Is>{}).construct_fill(begin, end, std::forward<Ps>(ps)), 0)...};
		}
		template <std::size_t ...Is, typename ...Ps>
		void construct_fill_impl(mpl::index_sequence<Is...>, std::ptrdiff_t begin, std::ptrdiff_t end, std::piecewise_construct_t, Ps && ...ps)
		{
			int expansion_hack[] = {(utl::unpack(std::forward<Ps>(ps), [this, begin, end](auto && ...ps){ section(mpl::index_constant<Is>{}).construct_fill(begin, end, std::forward<decltype(ps)>(ps)...); }), 0)...};
		}

		template <std::size_t ...Is, typename InputIt>
		void construct_range_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			int expansion_hack[] = {(section(mpl::index_constant<Is>{}).construct_range(index, utility::get<Is>(begin), utility::get<Is>(end)), 0)...};
		}
		template <std::size_t ...Is, typename InputIt>
		void construct_range_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index, std::move_iterator<InputIt> begin, std::move_iterator<InputIt> end)
		{
			int expansion_hack[] = {(section(mpl::index_constant<Is>{}).construct_range(index, std::make_move_iterator(utility::get<Is>(begin.base())), std::make_move_iterator(utility::get<Is>(end.base()))), 0)...};
		}

		template <typename ...Ps>
		reference construct_at_impl(mpl::index_sequence<0>, std::ptrdiff_t index, Ps && ...ps)
		{
			static_assert(value_types::size == 1, "");
			return reference(section(mpl::index_constant<0>{}).construct_at(index, std::forward<Ps>(ps)...));
		}
		template <std::size_t ...Is, typename P,
		          REQUIRES((utility::is_proxy_reference<mpl::remove_cvref_t<P>>::value))>
		reference construct_at_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index, P && p)
		{
			return reference(section(mpl::index_constant<Is>{}).construct_at(index, utility::get<Is>(std::forward<P>(p)))...);
		}
		template <std::size_t ...Is, typename ...Ps,
		          REQUIRES((sizeof...(Ps) != 1 || !utility::is_proxy_reference<mpl::remove_cvref_t<mpl::car<Ps...>>>::value))>
		reference construct_at_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index, Ps && ...ps)
		{
			return reference(section(mpl::index_constant<Is>{}).construct_at(index, std::forward<Ps>(ps))...);
		}
		// crashes clang 4.0
		// template <std::size_t ...Is, typename ...Ps>
		// reference piecewise_construct_at_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index, Ps && ...ps)
		// {
		// 	return reference(utl::unpack(std::forward<Ps>(ps), [this, index](auto && ...ps){ return section(mpl::index_constant<Is>{}).construct_at(index, std::forward<decltype(ps)>(ps)...); })...);
		// }
		template <typename ...Ps>
		reference piecewise_construct_at_impl(mpl::index_sequence<>, std::ptrdiff_t /*index*/, Ps && ...ps)
		{
			return reference(std::forward<Ps>(ps)...);
		}
		template <std::size_t I, typename P1, std::size_t ...Is>
		decltype(auto) construct_at_impl_helper(mpl::index_constant<I>, std::ptrdiff_t index, P1 && p1, mpl::index_sequence<Is...>)
		{
			return section(mpl::index_constant<I>{}).construct_at(index, std::get<Is>(std::forward<P1>(p1))...);
		}
		template <std::size_t I, std::size_t ...Is, typename P1, typename ...Ps>
		reference piecewise_construct_at_impl(mpl::index_sequence<I, Is...>, std::ptrdiff_t index, P1 && p1, Ps && ...ps)
		{
			// return construct_at_impl(mpl::index_sequence<Is...>{}, index, std::piecewise_construct, std::forward<Ps>(ps)..., utl::unpack(std::forward<P1>(p1), [this, index](auto && ...ps) { return section(mpl::index_constant<I>{}).construct_at(index, std::forward<decltype(ps)>(ps)...); }));
			return piecewise_construct_at_impl(mpl::index_sequence<Is...>{}, index, std::forward<Ps>(ps)..., construct_at_impl_helper(mpl::index_constant<I>{}, index, std::forward<P1>(p1), mpl::make_index_sequence<std::tuple_size<mpl::remove_cvref_t<P1>>::value>{}));
		}

		template <std::size_t ...Is>
		void destruct_range_impl(mpl::index_sequence<Is...>, std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			int expansion_hack[] = {(section(mpl::index_constant<Is>{}).destruct_range(begin, end), 0)...};
		}

		template <std::size_t ...Is>
		void destruct_at_impl(mpl::index_sequence<Is...>, std::ptrdiff_t index)
		{
			int expansion_hack[] = {(section(mpl::index_constant<Is>{}).destruct_at(index), 0)...};
		}
	};

	template <typename Storage>
	class const_storage_data
	{
	private:
		using value_types = typename Storage::value_types;

	public:
		using const_iterator = const_storage_iterator<Storage>;

		using const_reference = typename const_iterator::reference;

	private:
		const_iterator ptr_;

	public:
		const_storage_data(const_iterator && ptr)
			: ptr_(std::move(ptr))
		{}

	public:
		const_reference operator [] (std::ptrdiff_t index) const
		{
			return ptr_[index];
		}

		const_iterator data() const { return ptr_; }

		template <std::size_t I,
		          REQUIRES((I < value_types::size))>
		utility::const_section<Storage, typename Storage::template value_type_at<I>> section(mpl::index_constant<I>) const
		{
			return utility::const_section<Storage, typename Storage::template value_type_at<I>>(utility::get<I>(ptr_));
		}
	};

	namespace detail
	{
		template <std::size_t Capacity, typename ...Ts>
		class static_storage_impl
		{
		private:
			using this_type = static_storage_impl<Capacity, Ts...>;

		public:
			using value_types = mpl::type_list<Ts...>;

			template <std::size_t I>
			using value_type_at = mpl::type_at<I, Ts...>;
			template <typename Storing>
			using value_type_for = typename Storing::value_type;
			template <typename T>
			using storing_type_for = utility::storing<T>;

			using allocator_type = utility::null_allocator<char>;

			using iterator = utility::storage_iterator<this_type>;
			using const_iterator = utility::const_storage_iterator<this_type>;

		private:
			utility::tuple<std::array<storing_type_for<Ts>, Capacity>...> arrays;

		public:
			bool allocate(std::size_t capacity)
			{
				return capacity <= Capacity; // todo ==
			}

			void deallocate(std::size_t capacity)
			{
				assert(capacity <= Capacity); // todo ==
			}

			constexpr std::size_t max_size() const { return Capacity; }

			template <typename StoringType, typename ...Ps>
			typename StoringType::value_type & construct_at(StoringType * data_, std::ptrdiff_t index, Ps && ...ps)
			{
				assert((0 <= index && index < Capacity));
				return data_[index].construct(std::forward<Ps>(ps)...);
			}

			template <typename StoringType>
			void destruct_at(StoringType * data_, std::ptrdiff_t index)
			{
				assert((0 <= index && index < Capacity));
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

			template <std::size_t I>
			utility::section<this_type, value_type_at<I>> section(mpl::index_constant<I>)
			{
				return utility::section<this_type, value_type_at<I>>(utility::section_iterator<this_type, value_type_at<I>>(*this, get<I>(arrays).data()));
			}
			template <std::size_t I>
			utility::const_section<this_type, value_type_at<I>> section(mpl::index_constant<I>) const
			{
				return utility::const_section<this_type, value_type_at<I>>(utility::const_section_iterator<this_type, value_type_at<I>>(*this, get<I>(arrays).data()));
			}
			template <std::size_t I>
			utility::section<this_type, value_type_at<I>> section(mpl::index_constant<I> i, std::size_t /*capacity*/)
			{
				return section(i);
			}
			template <std::size_t I>
			utility::const_section<this_type, value_type_at<I>> section(mpl::index_constant<I> i, std::size_t /*capacity*/) const
			{
				return section(i);
			}

			utility::storage_data<this_type> sections()
			{
				return sections_impl(mpl::make_index_sequence_for<Ts...>{});
			}
			utility::const_storage_data<this_type> sections() const
			{
				return sections_impl(mpl::make_index_sequence_for<Ts...>{});
			}
			utility::storage_data<this_type> sections(std::size_t /*capacity*/)
			{
				return sections();
			}
			utility::const_storage_data<this_type> sections(std::size_t /*capacity*/) const
			{
				return sections();
			}
		private:
			template <std::size_t ...Is>
			utility::storage_data<this_type> sections_impl(mpl::index_sequence<Is...>)
			{
				return utility::storage_data<this_type>(utility::storage_iterator<this_type>(*this, get<Is>(arrays).data()...));
			}
			template <std::size_t ...Is>
			utility::const_storage_data<this_type> sections_impl(mpl::index_sequence<Is...>) const
			{
				return utility::const_storage_data<this_type>(utility::const_storage_iterator<this_type>(*this, get<Is>(arrays).data()...));
			}
		};

		template <template <typename> class Allocator, typename ...Ts>
		class dynamic_storage_impl
		{
		private:
			using this_type = dynamic_storage_impl<Allocator, Ts...>;

		public:
			using value_types = mpl::type_list<Ts...>;

			template <std::size_t I>
			using value_type_at = mpl::type_at<I, Ts...>;
			template <typename Storing>
			using value_type_for = Storing;
			template <typename T>
			using storing_type_for = T;

			using allocator_type = utility::aggregation_allocator<Allocator, void, Ts...>;

			using iterator = utility::storage_iterator<this_type>;
			using const_iterator = utility::const_storage_iterator<this_type>;
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

			template <std::size_t I>
			utility::section<this_type, value_type_at<I>> section(mpl::index_constant<I>, std::size_t capacity)
			{
#if MODE_DEBUG
				assert(storage() || capacity == 0);
#endif
				// there are times in which we will call allocator address
				// with a garbage storage, but maybe that is okay as long
				// as we never try to use that address?
				return utility::section<this_type, value_type_at<I>>(utility::section_iterator<this_type, value_type_at<I>>(*this, allocator().template address<I>(storage(), capacity)));
			}
			template <std::size_t I>
			utility::const_section<this_type, value_type_at<I>> section(mpl::index_constant<I>, std::size_t capacity) const
			{
#if MODE_DEBUG
				assert(storage() || capacity == 0);
#endif
				// ditto
				return utility::const_section<this_type, value_type_at<I>>(utility::const_section_iterator<this_type, value_type_at<I>>(*this, allocator().template address<I>(storage(), capacity)));
			}
			utility::section<this_type, value_type_at<0>> section(mpl::index_constant<0> i)
			{
				// we do not need the capacity to compute the first section
				return section(i, 0);
			}
			utility::const_section<this_type, value_type_at<0>> section(mpl::index_constant<0> i) const
			{
				// ditto
				return section(i, 0);
			}

			utility::storage_data<this_type> sections(std::size_t capacity)
			{
				return sections_impl(mpl::make_index_sequence_for<Ts...>{}, capacity);
			}
			utility::const_storage_data<this_type> sections(std::size_t capacity) const
			{
				return sections_impl(mpl::make_index_sequence_for<Ts...>{}, capacity);
			}
		private:
			allocator_type & allocator() { return impl_; }
			const allocator_type & allocator() const { return impl_; }

			void * & storage() { return impl_.storage_; }
			const void * storage() const { return impl_.storage_; }

			template <std::size_t ...Is>
			utility::storage_data<this_type> sections_impl(mpl::index_sequence<Is...>, std::size_t capacity)
			{
				return utility::storage_data<this_type>(utility::storage_iterator<this_type>(*this, allocator().template address<Is>(storage(), capacity)...));
			}
			template <std::size_t ...Is>
			utility::const_storage_data<this_type> sections_impl(mpl::index_sequence<Is...>, std::size_t capacity) const
			{
				return utility::const_storage_data<this_type>(utility::const_storage_iterator<this_type>(*this, allocator().template address<Is>(storage(), capacity)...));
			}
		};
	}

	template <typename StorageImpl, int = StorageImpl::value_types::size>
	class basic_storage
		: public StorageImpl
	{
	public:
		template <std::size_t I>
		using value_type_at = typename StorageImpl::template value_type_at<I>;
		template <typename T>
		using storing_type_for = typename StorageImpl::template storing_type_for<T>;

		using value_type = typename StorageImpl::iterator::value_type;
		using reference = typename StorageImpl::iterator::reference;
		using const_reference = typename StorageImpl::const_iterator::reference;
		using rvalue_reference = typename StorageImpl::iterator::rvalue_reference;

		using value_types = typename StorageImpl::value_types;
		using storing_types = mpl::transform<storing_type_for, value_types>;

		using storing_trivially_copyable = mpl::apply<mpl::conjunction,
		                                              mpl::transform<std::is_trivially_copyable,
		                                                             storing_types>>;
		using storing_trivially_destructible = mpl::apply<mpl::conjunction,
		                                                  mpl::transform<std::is_trivially_destructible,
		                                                                 storing_types>>;
	};

	template <typename StorageImpl>
	class basic_storage<StorageImpl, 1>
		: public StorageImpl
	{
	public:
		template <std::size_t I>
		using value_type_at = typename StorageImpl::template value_type_at<I>;
		template <typename T>
		using storing_type_for = typename StorageImpl::template storing_type_for<T>;

		using value_type = value_type_at<0>;
		using reference = value_type &;
		using const_reference = const value_type &;
		using rvalue_reference = value_type &&;

		using storing_type = storing_type_for<value_type>;
		using storing_trivially_copyable = std::is_trivially_copyable<storing_type>;
		using storing_trivially_destructible = std::is_trivially_destructible<storing_type>;

		using allocator_type = typename StorageImpl::allocator_type;
	private:
		template <typename InputIt>
		using can_memcpy = mpl::conjunction<storing_trivially_copyable,
		                                    utility::is_contiguous_iterator<InputIt>>;

	public:
		value_type & operator [] (std::ptrdiff_t index)
		{
			return single_section()[index];
		}
		const value_type & operator [] (std::ptrdiff_t index) const
		{
			return single_section()[index];
		}

		template <typename ...Ps>
		void construct_fill(std::ptrdiff_t begin, std::ptrdiff_t end, Ps && ...ps)
		{
			single_section().construct_fill(begin, end, std::forward<Ps>(ps)...);
		}

		template <typename InputIt>
		void construct_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			single_section().construct_range(index, begin, end);
		}

		using StorageImpl::construct_at;
		template <typename ...Ps>
		value_type & construct_at(std::ptrdiff_t index, Ps && ...ps)
		{
			return single_section().construct_at(index, std::forward<Ps>(ps)...);
		}

		template <typename InputIt,
		          REQUIRES((can_memcpy<InputIt>::value))>
		void memcpy_range(std::ptrdiff_t index, InputIt begin, InputIt end)
		{
			single_section().memcpy_range(index, begin, end);
		}

		using StorageImpl::data;
		value_type * data()
		{
			return single_section().data();
		}
		const value_type * data() const
		{
			return single_section().data();
		}

		void destruct_range(std::ptrdiff_t begin, std::ptrdiff_t end)
		{
			for (; begin != end; begin++)
			{
				single_section().destruct_at(begin);
			}
		}

		using StorageImpl::destruct_at;
		void destruct_at(std::size_t index)
		{
			single_section().destruct_at(index);
		}

		using StorageImpl::index_of;
		std::ptrdiff_t index_of(const value_type & x)
		{
			return single_section().index_of(x);
		}
	private:
		StorageImpl & base() { return static_cast<StorageImpl &>(*this); }
		const StorageImpl & base() const { return static_cast<const StorageImpl &>(*this); }

		section<StorageImpl, value_type> single_section() { return base().section(mpl::index_constant<0>{}); }
		const_section<StorageImpl, value_type> single_section() const { return base().section(mpl::index_constant<0>{}); }
	};

	template <template <typename> class Allocator, typename ...Ts>
	using dynamic_storage = basic_storage<detail::dynamic_storage_impl<Allocator, Ts...>>;
	template <std::size_t Capacity, typename ...Ts>
	using static_storage = basic_storage<detail::static_storage_impl<Capacity, Ts...>>;

	template <typename ...Ts>
	using heap_storage = dynamic_storage<utility::heap_allocator, Ts...>;

	template <typename Storage>
	struct storage_traits;
	template <template <typename> class Allocator, typename ...Ts>
	struct storage_traits<dynamic_storage<Allocator, Ts...>>
	{
		template <typename T>
		using allocator_type = Allocator<T>;
		template <typename ...Us>
		using storage_type = dynamic_storage<Allocator, Us...>;

		using static_capacity = mpl::false_type;
		using trivial_allocate = mpl::false_type;
		using trivial_deallocate = mpl::false_type;
		using moves_allocation = mpl::true_type;

		static std::size_t grow(std::size_t capacity, std::size_t amount)
		{
			assert(amount <= 8 || amount <= capacity);
			return capacity < 8 ? 8 : capacity * 2;
		}

		static std::size_t capacity_for(std::size_t size)
		{
			return size;
		}
	};
	template <std::size_t Capacity, typename ...Ts>
	struct storage_traits<static_storage<Capacity, Ts...>>
	{
		template <typename T>
		using allocator_type = utility::null_allocator<T>;
		template <typename ...Us>
		using storage_type = static_storage<Capacity, Us...>;

		using static_capacity = mpl::true_type;
		using trivial_allocate = mpl::true_type;
		using trivial_deallocate = mpl::true_type;
		using moves_allocation = mpl::false_type;

		static constexpr std::size_t capacity_value = Capacity;

		static constexpr std::size_t grow(std::size_t /*capacity*/, std::size_t /*amount*/)
		{
			return capacity_value;
		}

		static constexpr std::size_t capacity_for(std::size_t /*size*/)
		{
			return capacity_value;
		}
	};

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
	using storage_is_copy_constructible = std::is_copy_constructible<Storage>;
	template <typename Storage>
	using storage_is_copy_assignable = std::is_copy_assignable<Storage>;
	template <typename Storage>
	using storage_is_trivially_move_constructible =
		mpl::conjunction<std::is_move_constructible<Storage>,
		                 mpl::negation<typename storage_traits<Storage>::moves_allocation>>;
	template <typename Storage>
	using storage_is_trivially_move_assignable =
		mpl::conjunction<std::is_move_assignable<Storage>,
		                 mpl::negation<typename storage_traits<Storage>::moves_allocation>>;
}

#endif /* UTILITY_STORAGE_HPP */
