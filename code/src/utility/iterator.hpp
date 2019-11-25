
#ifndef UTILITY_ITERATOR_HPP
#define UTILITY_ITERATOR_HPP

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
		template <typename C>
		decltype(auto) begin(C & c)
		{
			using std::begin;

			return begin(c);
		}
		template <typename C>
		decltype(auto) end(C & c)
		{
			using std::end;

			return end(c);
		}
	}

	template <typename T>
	struct is_contiguous_container :
		mpl::conjunction<is_contiguous_iterator<decltype(detail::begin(std::declval<T &>()))>,
		                 is_contiguous_iterator<decltype(detail::end(std::declval<T &>()))>> {};
	template <typename T, std::size_t N>
	struct is_contiguous_container<T[N]> : mpl::true_type {};
	template <typename T, std::size_t N>
	struct is_contiguous_container<std::array<T, N>> : mpl::true_type {};
	template <typename T>
	struct is_contiguous_container<std::valarray<T>> : mpl::true_type {};
	template <typename T, typename Allocator>
	struct is_contiguous_container<std::vector<T, Allocator>> : mpl::true_type {};

	template <typename C,
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto begin(C & c)
	{
		return std::begin(c);
	}
	template <typename C,
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto begin(C & c)
	{
		return make_contiguous_iterator(std::begin(c));
	}
	template <typename C>
	constexpr auto begin(C && c)
	{
		return std::make_move_iterator(utility::begin(c));
	}

	template <typename C>
	constexpr auto cbegin(const C & c)
	{
		return utility::begin(c);
	}
	template <typename C>
	constexpr auto cbegin(const C && c)
	{
		return utility::begin(std::move(c));
	}

	template <typename C,
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rbegin(C & c)
	{
		return std::rbegin(c);
	}
	template <typename C,
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rbegin(C & c)
	{
		return make_contiguous_iterator(std::rbegin(c));
	}
	template <typename C>
	constexpr auto rbegin(C && c)
	{
		return std::make_move_iterator(utility::rbegin(c));
	}

	template <typename C>
	constexpr auto crbegin(const C & c)
	{
		return utility::rbegin(c);
	}
	template <typename C>
	constexpr auto crbegin(const C && c)
	{
		return utility::rbegin(std::move(c));
	}

	template <typename C,
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto end(C & c)
	{
		return std::end(c);
	}
	template <typename C,
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto end(C & c)
	{
		return make_contiguous_iterator(std::end(c));
	}
	template <typename C>
	constexpr auto end(C && c)
	{
		return std::make_move_iterator(utility::end(c));
	}

	template <typename C>
	constexpr auto cend(const C & c)
	{
		return utility::end(c);
	}
	template <typename C>
	constexpr auto cend(const C && c)
	{
		return utility::end(std::move(c));
	}

	template <typename C,
	          REQUIRES((!is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rend(C & c)
	{
		return std::rend(c);
	}
	template <typename C,
	          REQUIRES((is_contiguous_container<std::remove_const_t<C>>::value))>
	constexpr auto rend(C & c)
	{
		return make_contiguous_iterator(std::rend(c));
	}
	template <typename C>
	constexpr auto rend(C && c)
	{
		return std::make_move_iterator(utility::rend(c));
	}

	template <typename C>
	constexpr auto crend(const C & c)
	{
		return utility::rend(c);
	}
	template <typename C>
	constexpr auto crend(const C && c)
	{
		return utility::rend(std::move(c));
	}


	template <typename T>
	struct is_pair : mpl::false_type {};
	template <typename ...Ts>
	struct is_pair<std::pair<Ts...>> : mpl::true_type {};

	template <typename T>
	struct is_tuple : mpl::false_type {};
	template <typename ...Ts>
	struct is_tuple<std::tuple<Ts...>> : mpl::true_type {};


	template <typename ...Rs>
	class proxy_reference;

	template <typename T>
	struct is_proxy_reference : mpl::false_type {};
	template <typename ...Rs>
	struct is_proxy_reference<proxy_reference<Rs...>> : mpl::true_type {};

	template <typename T>
	struct proxy_reference_size;
	template <typename ...Rs>
	struct proxy_reference_size<proxy_reference<Rs...>> : mpl::index_constant<sizeof...(Rs)> {};

	namespace detail
	{
		template <std::size_t N>
		struct proxy_reference_base_impl
		{
			template <typename ...Rs>
			using type = std::tuple<Rs...>;
		};
		template <>
		struct proxy_reference_base_impl<2>
		{
			template <typename R1, typename R2>
			using type = std::pair<R1, R2>;
		};
		template <typename ...Rs>
		using proxy_reference_base = typename proxy_reference_base_impl<sizeof...(Rs)>::template type<Rs...>;
	}

	// inspired by the works of eric niebler
	//
	// http://ericniebler.com/2015/02/13/iterators-plus-plus-part-2/

	template <typename ...Rs>
	class proxy_reference
		: public detail::proxy_reference_base<Rs...>
	{
		template <typename ...Ss>
		friend class proxy_reference;

		using this_type = proxy_reference<Rs...>;
		using base_type = detail::proxy_reference_base<Rs...>;

	public:
		proxy_reference() = default;
		explicit proxy_reference(Rs && ...rs)
			: base_type(std::forward<Rs>(rs)...)
		{}
		template <typename Other,
		          typename OtherT = mpl::remove_cvref_t<Other>,
		          REQUIRES((is_proxy_reference<OtherT>::value)),
		          REQUIRES((!mpl::is_same<this_type, OtherT>::value)),
		          REQUIRES((proxy_reference_size<OtherT>::value == sizeof...(Rs)))>
		proxy_reference(Other && other)
			: proxy_reference(mpl::make_index_sequence_for<Rs...>{}, std::forward<Other>(other))
		{}
		template <typename Other,
		          typename OtherT = mpl::remove_cvref_t<Other>,
		          REQUIRES((is_pair<OtherT>::value || is_tuple<OtherT>::value)),
		          REQUIRES((std::tuple_size<OtherT>::value == sizeof...(Rs))),
		          REQUIRES((mpl::is_constructible_from_tuple<base_type, Other &&>::value))>
		proxy_reference(Other && other)
			: proxy_reference(mpl::make_index_sequence_for<Rs...>{}, std::forward<Other>(other))
		{}
		template <typename Other,
		          typename OtherT = mpl::remove_cvref_t<Other>,
		          REQUIRES((is_proxy_reference<OtherT>::value)),
		          REQUIRES((!mpl::is_same<this_type, OtherT>::value)),
		          REQUIRES((proxy_reference_size<OtherT>::value == sizeof...(Rs)))>
		this_type & operator = (Other && other)
		{
			return operator_equal(mpl::make_index_sequence_for<Rs...>{}, std::forward<Other>(other));
		}
		template <typename Other,
		          typename OtherT = mpl::remove_cvref_t<Other>,
		          REQUIRES((is_pair<OtherT>::value || is_tuple<OtherT>::value)),
		          REQUIRES((std::tuple_size<OtherT>::value == sizeof...(Rs))),
		          REQUIRES((mpl::is_constructible_from_tuple<base_type, Other &&>::value))>
		this_type & operator = (Other && other)
		{
			return operator_equal(mpl::make_index_sequence_for<Rs...>{}, std::forward<Other>(other));
		}
	private:
		template <std::size_t ...Is, typename Tuple>
		proxy_reference(mpl::index_sequence<Is...>, Tuple && tuple)
			: base_type(std::get<Is>(std::forward<Tuple>(tuple))...)
		{}
		template <std::size_t ...Is, typename Tuple>
		this_type & operator_equal(mpl::index_sequence<Is...>, Tuple && tuple)
		{
			int expansion_hack[] = {(std::get<Is>(*this) = std::get<Is>(std::forward<Tuple>(tuple)), 0)...};
			return *this;
		}

	public:
		template <typename T,
		          REQUIRES((sizeof...(Rs) == utility::int_hack<T, 1>::value)),
		          REQUIRES((std::is_convertible<mpl::car<Rs...>, T &>::value))>
		operator T & () &
		{
			return std::get<0>(*this);
		}
		template <typename T,
		          REQUIRES((sizeof...(Rs) == utility::int_hack<T, 1>::value)),
		          REQUIRES((std::is_convertible<mpl::car<Rs...>, T &&>::value))>
		operator T && () &&
		{
			return std::move(std::get<0>(*this));
		}
	};

	template <std::size_t I, typename That,
	          REQUIRES((is_proxy_reference<mpl::remove_cvref_t<That>>::value))>
	decltype(auto) get(That && that) { return std::get<I>(std::forward<That>(that)); }
	template <typename T, typename That,
	          REQUIRES((is_proxy_reference<mpl::remove_cvref_t<That>>::value))>
	decltype(auto) get(That && that) { return std::get<T>(std::forward<That>(that)); }

	template <typename ...Ps>
	auto make_proxy_reference(Ps && ...ps)
	{
		return utility::proxy_reference<Ps &&...>(std::forward<Ps>(ps)...);
	}
}

#endif /* UTILITY_ITERATOR_HPP */
