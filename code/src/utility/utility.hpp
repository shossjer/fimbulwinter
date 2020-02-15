
#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <utility/concepts.hpp>
#include <utility/type_traits.hpp>

#include <utility>

namespace utility
{
	struct in_place_t
	{
		explicit in_place_t() = default;
	};
	constexpr in_place_t in_place = in_place_t{};

	template <size_t>
	struct in_place_index_helper {};
	template <size_t N>
	in_place_index_helper<N> in_place_index() { return {}; }
	template <size_t N>
	using in_place_index_t = in_place_index_helper<N>();

	template <typename T>
	struct is_in_place_index : mpl::false_type {};
	template <size_t I>
	struct is_in_place_index<in_place_index_t<I>> : mpl::true_type {};

	template <typename T>
	struct in_place_type_t { explicit constexpr in_place_type_t(int) {} };
	template <typename T>
	constexpr in_place_type_t<T> in_place_type = in_place_type_t<T>(0);

	template <typename T>
	struct is_in_place_type : mpl::false_type {};
	template <typename T>
	struct is_in_place_type<in_place_type_t<T>> : mpl::true_type {};

	struct monostate
	{
		explicit monostate() = default;
	};

	struct null_place_t { explicit constexpr null_place_t(int) {} };
	constexpr null_place_t null_place = null_place_t(0);

#if defined(_MSC_VER) && _MSC_VER <= 1916
	template <typename T, typename ...Ps>
	mpl::enable_if_t<mpl::is_paren_constructible<T, Ps...>::value, T>
	construct(Ps && ...ps)
	{
		return T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps>
	mpl::enable_if_t<!mpl::is_paren_constructible<T, Ps...>::value && mpl::is_brace_constructible<T, Ps...>::value, T>
	construct(Ps && ...ps)
	{
		return T{std::forward<Ps>(ps)...};
	}

	template <typename T, typename ...Ps>
	mpl::enable_if_t<mpl::is_paren_constructible<T, Ps...>::value, T &>
	construct_at(void * ptr, Ps && ...ps)
	{
		return *new (ptr) T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps>
	mpl::enable_if_t<!mpl::is_paren_constructible<T, Ps...>::value && mpl::is_brace_constructible<T, Ps...>::value, T &>
	construct_at(void * ptr, Ps && ...ps)
	{
		return *new (ptr) T{std::forward<Ps>(ps)...};
	}

	template <typename T, typename ...Ps>
	mpl::enable_if_t<mpl::is_paren_constructible<T, Ps...>::value, T *>
	construct_new(Ps && ...ps)
	{
		return new T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps>
	mpl::enable_if_t<!mpl::is_paren_constructible<T, Ps...>::value && mpl::is_brace_constructible<T, Ps...>::value, T *>
	construct_new(Ps && ...ps)
	{
		return new T{std::forward<Ps>(ps)...};
	}
#else
	template <typename T, typename ...Ps,
	          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
	T construct(Ps && ...ps)
	{
		return T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
	T construct(Ps && ...ps)
	{
		return T{std::forward<Ps>(ps)...};
	}
	template <typename T, typename U = typename T::value_type, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((!mpl::is_brace_constructible<T, Ps...>::value))>
	T construct(Ps && ...ps)
	{
		return T{static_cast<U>(std::forward<Ps>(ps))...};
	}

	template <typename T, typename ...Ps,
	          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
	T & construct_at(void * ptr, Ps && ...ps)
	{
		return *new (ptr) T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
	T & construct_at(void * ptr, Ps && ...ps)
	{
		return *new (ptr) T{std::forward<Ps>(ps)...};
	}
	template <typename T, typename U = typename T::value_type, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((!mpl::is_brace_constructible<T, Ps...>::value))>
	T & construct_at(void * ptr, Ps && ...ps)
	{
		return *new (ptr) T{static_cast<U>(std::forward<Ps>(ps))...};
	}

	template <typename T, typename ...Ps,
	          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
	T * construct_new(Ps && ...ps)
	{
		return new T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
	T * construct_new(Ps && ...ps)
	{
		return new T{std::forward<Ps>(ps)...};
	}
#endif

	template <typename T, int N>
	struct int_hack : mpl::integral_constant<int, N> {};
}

#endif /* UTILITY_UTILITY_HPP */
