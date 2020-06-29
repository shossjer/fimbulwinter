#pragma once

#include "utility/algorithm.hpp"
#include "utility/compound.hpp"
#include "utility/concepts.hpp"
#include "utility/intrinsics.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

#include <array>
#include <iterator>
#include <string>
#include <tuple>
#include <utility>
#include <valarray>
#include <vector>

namespace utility
{
	template <typename Iter>
	class contiguous_iterator
	{
	public:
		using iterator_type = Iter;
		using value_type = typename std::iterator_traits<Iter>::value_type;
		using difference_type = typename std::iterator_traits<Iter>::difference_type;
		using pointer = typename std::iterator_traits<Iter>::pointer;
		using reference = typename std::iterator_traits<Iter>::reference;
		using iterator_category = typename std::iterator_traits<Iter>::iterator_category;

	private:
		iterator_type current;

	public:
		constexpr contiguous_iterator() = default;
		constexpr explicit contiguous_iterator(iterator_type x)
			: current(x)
		{}
		template <typename Iter2>
		constexpr contiguous_iterator(const contiguous_iterator<Iter2> & x)
			: current(x.base())
		{}

		template <typename Iter2>
		contiguous_iterator & operator = (const contiguous_iterator<Iter2> & x)
		{
			current = x.base();
			return *this;
		}

	public:
		constexpr iterator_type base() const { return current; }

		constexpr reference operator * () const { return *current; }
		constexpr pointer operator -> () const { return std::addressof(*current); }

		constexpr auto operator [] (difference_type n) const { return current[n]; }

		constexpr contiguous_iterator & operator ++ () { ++current; return *this; }
		constexpr contiguous_iterator & operator -- () { --current; return *this; }
		constexpr contiguous_iterator operator ++ (int) { return contiguous_iterator(current++); }
		constexpr contiguous_iterator operator -- (int) { return contiguous_iterator(current--); }
		constexpr contiguous_iterator operator + (difference_type n) { return contiguous_iterator(current + n); }
		constexpr contiguous_iterator operator - (difference_type n) { return contiguous_iterator(current - n); }
		constexpr contiguous_iterator & operator += (difference_type n) { current += n; return *this; }
		constexpr contiguous_iterator & operator -= (difference_type n) { current -= n; return *this; }

		friend constexpr contiguous_iterator operator + (difference_type n, const contiguous_iterator & x) { return x + n; }
	};

	template <typename Iter1, typename Iter2>
	constexpr bool operator == (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() == i2.base();
	}
	template <typename Iter1, typename Iter2>
	constexpr bool operator != (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() != i2.base();
	}
	template <typename Iter1, typename Iter2>
	constexpr bool operator < (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() < i2.base();
	}
	template <typename Iter1, typename Iter2>
	constexpr bool operator <= (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() <= i2.base();
	}
	template <typename Iter1, typename Iter2>
	constexpr bool operator > (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() > i2.base();
	}
	template <typename Iter1, typename Iter2>
	constexpr bool operator >= (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() >= i2.base();
	}

	template <typename Iter1, typename Iter2>
	constexpr auto operator - (const contiguous_iterator<Iter1> & i1, const contiguous_iterator<Iter2> & i2)
	{
		return i1.base() - i2.base();
	}

	template <typename T>
	constexpr T * make_contiguous_iterator(T * i)
	{
		return i;
	}
	template <typename Iter>
	constexpr contiguous_iterator<Iter> make_contiguous_iterator(Iter i)
	{
		return contiguous_iterator<Iter>(i);
	}

	template <typename T>
	constexpr auto to_address(T * ptr) { return ptr; }
	template <typename It>
	constexpr auto to_address(const It & it)
	{
		return to_address(it.operator->());
	}

	template <typename T>
	constexpr std::pair<T *, T *> raw_range(T * begin, T * end)
	{
		return std::make_pair(begin, end);
	}
	template <typename It>
	decltype(auto) raw_range(std::reverse_iterator<It> begin, std::reverse_iterator<It> end)
	{
		return raw_range(++end.base(), ++begin.base());
	}
	template <typename It>
	decltype(auto) raw_range(It begin, It end)
	{
		// this might be fine to have as a default :shrug:
		return raw_range(begin.base(), end.base());
	}

	template <typename It>
	struct is_contiguous_iterator_impl
	{
		template <typename T>
		static mpl::true_type foo(T *, signed) { intrinsic_unreachable(); }
		template <typename BaseIt>
		static mpl::true_type foo(const contiguous_iterator<BaseIt> &, signed) { intrinsic_unreachable(); }
		template <typename T>
		static mpl::false_type foo(const T &, ...) { intrinsic_unreachable(); }
		template <typename T>
		static auto foo(const T & it, decltype(it.base(), unsigned())) { return foo(it.base(), 0); }

		using type = decltype(foo(std::declval<It>(), 0));
	};
	template <typename It>
	using is_contiguous_iterator = typename is_contiguous_iterator_impl<It>::type;

	namespace detail
	{
		template <typename T>
		auto is_range_impl(const T & x, int) -> decltype(std::begin(x), std::end(x), mpl::true_type());
		template <typename T>
		auto is_range_impl(const T &, ...) -> mpl::false_type;
	}

	template <typename T>
	using is_range = decltype(detail::is_range_impl(std::declval<T>(), 0));

	namespace detail
	{
		template <typename T, std::size_t N>
		auto is_contiguous_container_impl(const T (&)[N], int) -> mpl::true_type;
		template <typename T, std::size_t N>
		auto is_contiguous_container_impl(const std::array<T, N> &, int) -> mpl::true_type;
		template <typename T>
		auto is_contiguous_container_impl(const std::valarray<T> &, int) -> mpl::true_type;
		template <typename T, typename Allocator>
		auto is_contiguous_container_impl(const std::vector<T, Allocator> &, int) -> mpl::true_type;
		template <typename T>
		auto is_contiguous_container_impl(const T & x, int) -> mpl::conjunction<is_contiguous_iterator<decltype(std::begin(x))>, is_contiguous_iterator<decltype(std::end(x))>>;
		template <typename T>
		auto is_contiguous_container_impl(const T &, ...) -> mpl::false_type;
	}

	template <typename T>
	using is_contiguous_container = decltype(detail::is_contiguous_container_impl(std::declval<T>(), 0));

	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto begin(C & c)
	{
		return std::begin(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto begin(C & c)
	{
		return make_contiguous_iterator(std::begin(c));
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto begin(C && c)
	{
		return std::make_move_iterator(utility::begin(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto cbegin(const C & c)
	{
		return utility::begin(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto cbegin(const C && c)
	{
		return utility::begin(std::move(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rbegin(C & c)
	{
		return std::rbegin(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rbegin(C & c)
	{
		return make_contiguous_iterator(std::rbegin(c));
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto rbegin(C && c)
	{
		return std::make_move_iterator(utility::rbegin(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto crbegin(const C & c)
	{
		return utility::rbegin(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto crbegin(const C && c)
	{
		return utility::rbegin(std::move(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto end(C & c)
	{
		return std::end(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto end(C & c)
	{
		return make_contiguous_iterator(std::end(c));
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto end(C && c)
	{
		return std::make_move_iterator(utility::end(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto cend(const C & c)
	{
		return utility::end(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto cend(const C && c)
	{
		return utility::end(std::move(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rend(C & c)
	{
		return std::rend(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value)),
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rend(C & c)
	{
		return make_contiguous_iterator(std::rend(c));
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto rend(C && c)
	{
		return std::make_move_iterator(utility::rend(c));
	}

	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto crend(const C & c)
	{
		return utility::rend(c);
	}
	template <typename C,
	          REQUIRES((is_range<C>::value))>
	constexpr auto crend(const C && c)
	{
		return utility::rend(std::move(c));
	}

	template <typename It>
	class basic_range
	{
	public:
		using iterator = It;

	private:
		It begin_;
		It end_;

	public:
		explicit constexpr basic_range(It begin, It end)
			: begin_(begin)
			, end_(end)
		{}

	public:
		constexpr iterator begin() const { return begin_; }
		constexpr iterator end() const { return end_; }
	};

	template <typename R,
	          REQUIRES((is_range<R>::value))>
	auto reverse(R && range)
	{
		return basic_range<decltype(utility::rbegin(std::forward<R>(range)))>(utility::rbegin(std::forward<R>(range)), utility::rend(std::forward<R>(range)));
	}

	// eric niebler is a god :pray:
	//
	// http://ericniebler.com/2015/02/03/iterators-plus-plus-part-1/

	template <typename It,
	          typename R = typename std::iterator_traits<It>::reference>
	mpl::conditional_t<std::is_reference<R>::value,
	                   mpl::remove_reference_t<R> &&,
	                   R>
	iter_move(It it)
	{
		return std::move(*it);
	}

	template <typename ...Its>
	class zip_iterator
		: public utility::compound<Its...>
	{
		template <typename ...Jts>
		friend class zip_iterator;

		using this_type = zip_iterator<Its...>;
		using base_type = utility::compound<Its...>;

	public:
		using difference_type = typename std::iterator_traits<mpl::car<Its...>>::difference_type; // ?
		using value_type = utility::compound<typename std::iterator_traits<Its>::value_type...>;
		using pointer = utility::compound<typename std::iterator_traits<Its>::pointer...>;
		using reference = utility::compound<typename std::iterator_traits<Its>::reference...>;
		using iterator_category = typename std::iterator_traits<mpl::car<Its...>>::iterator_category; // ?

	public:
		using base_type::base_type;
		using base_type::operator =;

	public:
		reference operator * () const
		{
			return ext::apply([](auto & ...ps){ return reference(*ps...); }, *this);
		}

		reference operator [] (difference_type n) const { return ext::apply([n](auto & ...ps){ return reference(ps[n]...); }, *this); }

		this_type & operator ++ () { ext::apply([](auto & ...ps){ int expansion_hack[] = {(++ps, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type & operator -- () { ext::apply([](auto & ...ps){ int expansion_hack[] = {(--ps, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type operator ++ (int) { return ext::apply([](auto & ...ps){ return this_type(ps++...); }, *this); }
		this_type operator -- (int) { return ext::apply([](auto & ...ps){ return this_type(ps--...); }, *this); }
		this_type operator + (difference_type n) { return ext::apply([n](auto & ...ps){ return this_type(ps + n...); }, *this); }
		this_type operator - (difference_type n) { return ext::apply([n](auto & ...ps){ return this_type(ps - n...); }, *this); }
		this_type & operator += (difference_type n) { ext::apply([n](auto & ...ps){ int expansion_hack[] = {(ps += n, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }
		this_type & operator -= (difference_type n) { ext::apply([n](auto & ...ps){ int expansion_hack[] = {(ps -= n, 0)...}; static_cast<void>(expansion_hack); }, *this); return *this; }

		friend this_type operator + (difference_type n, const this_type & x) { return x + n; }

	private:
		friend auto iter_move(this_type x)
		{
			using utility::iter_move;
			return ext::apply([](auto & ...ps){ return utility::forward_as_compound(iter_move(ps)...); }, x);
		}
	};

	template <typename ...Its, typename ...Jts>
	bool operator == (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) == static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator != (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) != static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator < (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) < static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator <= (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) <= static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator > (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) > static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	bool operator >= (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return static_cast<const std::tuple<Its...> &>(i1) >= static_cast<const std::tuple<Its...> &>(i2);
	}

	template <typename ...Its, typename ...Jts>
	auto operator - (const zip_iterator<Its...> & i1, const zip_iterator<Jts...> & i2)
	{
		return std::get<0>(i1) - std::get<0>(i2);
	}

	template <typename ...Ptrs>
	class zip_pointer
		: public utility::zip_iterator<Ptrs...>
	{
		template <typename ...Ptrs_>
		friend class zip_pointer;

		using this_type = zip_pointer<Ptrs...>;
		using base_type = utility::zip_iterator<Ptrs...>;

	public:
		// note this is a great hack in order to get
		// `std::pointer_traits<>::pointer_to` to work for proxy types.
		using element_type = typename base_type::reference;

	public:
		static typename base_type::pointer pointer_to(typename base_type::reference element)
		{
			return ext::apply([](auto & ...ps){ return typename base_type::pointer(std::pointer_traits<Ptrs>::pointer_to(ps)...); }, element);
		}

	public:
		using base_type::base_type;
		using base_type::operator =;

		zip_pointer(std::nullptr_t)
			: base_type{}
		{}

	public:
		explicit operator bool () const { return static_cast<bool>(std::get<0>(*this)); } // todo

	private:
		friend bool operator == (const this_type & x, std::nullptr_t) { return !static_cast<bool>(x); }
		friend bool operator == (std::nullptr_t, const this_type & x) { return x == nullptr; }
		friend bool operator != (const this_type & x, std::nullptr_t) { return !(x == nullptr); }
		friend bool operator != (std::nullptr_t, const this_type & x) { return !(x == nullptr); }
	};
}
