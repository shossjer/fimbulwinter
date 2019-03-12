
#ifndef UTILITY_ITERATOR_HPP
#define UTILITY_ITERATOR_HPP

#include "concepts.hpp"
#include "intrinsics.hpp"
#include "type_traits.hpp"

#include <array>
#include <iterator>
#include <string>
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
		static mpl::true_type foo(T *, int) { intrinsic_unreachable(); }
		template <typename BaseIt>
		static mpl::true_type foo(const contiguous_iterator<BaseIt> &, int) { intrinsic_unreachable(); }
		template <typename T>
		static mpl::false_type foo(const T &, ...) { intrinsic_unreachable(); }
		template <typename T>
		static auto foo(const T & it, decltype(it.base(), int())) { return foo(it.base(), 0); }

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
}

#endif /* UTILITY_ITERATOR_HPP */
