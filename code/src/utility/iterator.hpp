
#ifndef UTILITY_ITERATOR_HPP
#define UTILITY_ITERATOR_HPP

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
		using difference_type = typename std::iterator_traits<mpl::car<Its...>>::difference_type;
		using reference = utility::compound<typename std::iterator_traits<Its>::reference...>;

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
}

#endif /* UTILITY_ITERATOR_HPP */
