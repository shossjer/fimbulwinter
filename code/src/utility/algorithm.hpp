#pragma once

#include "utility/concepts.hpp"
#include "utility/type_traits.hpp"

#include <algorithm>
#include <array>
#include <tuple>
#include <valarray>
#include <vector>

namespace ext
{
	namespace detail
	{
		struct not_this_type {};
	}
}

namespace std
{
	void size(ext::detail::not_this_type);
}

namespace ext
{
	template <typename ...Ps>
	static void declexpand(Ps && ...);

	namespace detail
	{
		template <typename T, std::size_t N>
		static auto tuple_size(const T (&)[N]) -> mpl::index_constant<N>;
		template <typename T, std::size_t N>
		static auto tuple_size(const std::array<T, N> &) -> mpl::index_constant<N>;
		template <typename T1, typename T2>
		static auto tuple_size(const std::pair<T1, T2> &) -> mpl::index_constant<2>;
		template <typename ...Ts>
		static auto tuple_size(const std::tuple<Ts...> &) -> mpl::index_constant<sizeof...(Ts)>;

		template <typename Tuple>
		static auto tuple_size_impl(const Tuple & tuple) -> decltype(tuple_size(tuple));
	}
	// c++11
	template <typename T>
	using tuple_size = decltype(detail::tuple_size_impl(std::declval<T>()));

	namespace detail
	{
		template <std::size_t I, typename T, std::size_t N>
		static auto tuple_element(const T (&)[N]) -> mpl::enable_if_t<(I < N), T>;
		template <std::size_t I, typename T, std::size_t N>
		static auto tuple_element(const std::array<T, N> &) -> mpl::enable_if_t<(I < N), T>;
		template <std::size_t I, typename T1, typename T2>
		static auto tuple_element(const std::pair<T1, T2> &) -> mpl::type_at<I, T1, T2>;
		template <std::size_t I, typename ...Ts>
		static auto tuple_element(const std::tuple<Ts...> &) -> mpl::type_at<I, Ts...>;

		template <std::size_t I, typename Tuple>
		static auto tuple_element_impl(const Tuple & tuple) -> decltype(tuple_element<I>(tuple));
	}
	// c++11
	template <std::size_t I, typename T>
	using tuple_element = decltype(detail::tuple_element_impl<I>(std::declval<T>()));

	namespace detail
	{
		template <typename Tuple>
		auto is_tuple_impl(int, Tuple && tuple) -> decltype(tuple_size(std::forward<Tuple>(tuple)), mpl::true_type());
		template <typename ...Ts>
		auto is_tuple_impl(float, Ts && ...) -> mpl::false_type;
	}
	template <typename ...Ts>
	using is_tuple = decltype(detail::is_tuple_impl(0, std::declval<Ts>()...));

	using std::get;

	template <std::size_t I, typename T, std::size_t N>
	mpl::enable_if_t<(I < N), T &> get(T (& array)[N]) { return array[I]; }
	template <std::size_t I, typename T, std::size_t N>
	mpl::enable_if_t<(I < N), const T &> get(const T (& array)[N]) { return array[I]; }
	template <std::size_t I, typename T, std::size_t N>
	mpl::enable_if_t<(I < N), T &&> get(T (&& array)[N]) { return static_cast<T &&>(array[I]); }
	template <std::size_t I, typename T, std::size_t N>
	mpl::enable_if_t<(I < N), const T &&> get(const T (&& array)[N]) { return static_cast<const T &&>(array[I]); }

	namespace detail
	{
		template <typename T>
		static auto has_std_size_impl(T && x, int) -> decltype(std::size(x), mpl::true_type());
		template <typename T>
		static auto has_std_size_impl(T &&, ...) -> mpl::false_type;
	}
	template <typename T>
	using has_std_size = decltype(detail::has_std_size_impl(std::declval<T>(), 0));

#if defined(_MSC_VER)
	// microsofts standard library defines this c++17 feature regardless of our targeted c++ version
	using std::size;
#else
	// c++17
	template <typename T, std::size_t N>
	constexpr std::size_t size(const T (&)[N]) { return N; }
	// note array is a tuple but it is not found by ADL in the std namespace, so we have to define it here

	// c++17
	template <typename C,
	          REQUIRES((!has_std_size<C>::value))> // note our implementation collides with the one in std
	constexpr auto size(const C & c) -> decltype(c.size()) { return c.size(); }
#endif

	template <typename Tuple,
	          REQUIRES((!has_std_size<Tuple>::value))> // note std::array is a tuple, so we have to guard against collisions here as well
	constexpr auto size(const Tuple &) -> decltype(ext::tuple_size<Tuple>(), std::size_t()) { return ext::tuple_size<Tuple>::value; }

	//
	template <typename T>
	using make_tuple_sequence = mpl::make_index_sequence<ext::tuple_size<T>::value>;

	template <typename F, typename Tuple, std::size_t ...Is>
	decltype(auto) apply_for(F && f, Tuple && tuple, mpl::index_sequence<Is...>)
	{
		return std::forward<F>(f)(ext::get<Is>(std::forward<Tuple>(tuple))...);
	}

	// c++17
	template <typename F, typename Tuple>
	decltype(auto) apply(F && f, Tuple && tuple)
	{
		return apply_for(std::forward<F>(f),
		                 std::forward<Tuple>(tuple),
		                 ext::make_tuple_sequence<Tuple>{});
	}
}

namespace mpl
{
	template <typename T, typename Tuple, typename Indices = ext::make_tuple_sequence<Tuple>>
	struct is_constructible_from_tuple;
	template <typename T, typename Tuple, std::size_t ...Is>
	struct is_constructible_from_tuple<T, Tuple, mpl::index_sequence<Is...>> : std::is_constructible<T, decltype(std::get<Is>(std::declval<Tuple>()))...> {};
}

namespace utl
{
	template <typename T, std::size_t N, std::size_t ...Is,
	          REQUIRES((std::is_constructible<T, T &>::value))>
	constexpr auto to_array_for(T (& a)[N], mpl::index_sequence<Is...>)
	{
		return std::array<mpl::remove_cv_t<T>, sizeof...(Is)>{{a[Is]...}};
	}

	template <typename T, std::size_t N, std::size_t ...Is,
	          REQUIRES((std::is_move_constructible<T>::value))>
	constexpr auto to_array_for(T (&& a)[N], mpl::index_sequence<Is...>)
	{
		return std::array<mpl::remove_cv_t<T>, sizeof...(Is)>{{std::move(a[Is])...}};
	}

	// c++20
	template <typename T, std::size_t N,
	          REQUIRES((std::is_constructible<T, T &>::value))>
	constexpr auto to_array(T (& a)[N])
	{
		return to_array_for(a, mpl::make_index_sequence<N>{});
	}

	// c++20
	template <typename T, std::size_t N,
	          REQUIRES((std::is_move_constructible<T>::value))>
	constexpr auto to_array(T (&& a)[N])
	{
		return to_array_for(std::move(a), mpl::make_index_sequence<N>{});
	}

	// c++??
	template <typename T = void, typename ...Ps>
	constexpr auto make_array(Ps && ...ps)
	{
		using value_type = mpl::common_type_unless_nonvoid<T, mpl::remove_cv_t<Ps>...>;

		return std::array<value_type, sizeof...(Ps)>{{std::forward<Ps>(ps)...}};
	}

	////////////////////////////////////////////////////////////////////////////
	//
	//  fundamental functions or something
	//
	//////////////////////////////////////////////////////////////////
	template <typename T, std::size_t N, std::size_t ...Is, typename F>
	inline auto map_for(const std::array<T, N> & array,
	                    mpl::index_sequence<Is...>,
	                    F && f) ->
		decltype(std::array<decltype(f(std::declval<T>())), N>{{f(std::get<Is>(array))...}})
	{
		return std::array<decltype(f(std::declval<T>())), N>{{f(std::get<Is>(array))...}};
	}
	template <typename ...Ts, std::size_t ...Is, typename F>
	inline auto map_for(const std::tuple<Ts...> & array,
	                    mpl::index_sequence<Is...>,
	                    F && f) ->
		decltype(std::make_tuple(f(std::get<Is>(array))...))
	{
		return std::make_tuple(f(std::get<Is>(array))...);
	}
	template <typename Array, typename F>
	inline auto map(Array && array, F && f) ->
		decltype(map_for(std::forward<Array>(array),
		                 ext::make_tuple_sequence<Array>{},
		                 std::forward<F>(f)))
	{
		return map_for(std::forward<Array>(array),
		               ext::make_tuple_sequence<Array>{},
		               std::forward<F>(f));
	}

#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
	// C4702 - unreachable code
#endif
	template <typename Array, std::size_t ...Is, typename F>
	void for_each_in(Array && array,
	                 mpl::index_sequence<Is...>,
	                 F && f)
	{
		int expansion_hack[] = {(f(std::get<Is>(array)), 0)...};
		static_cast<void>(expansion_hack);
	}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
	template <typename Array, typename F>
	void for_each(Array && array, F && f)
	{
		return for_each_in(std::forward<Array>(array),
		                   ext::make_tuple_sequence<Array>{},
		                   std::forward<F>(f));
	}

	////////////////////////////////////////////////////////////////////////////
	//
	//  kjhs
	//
	//////////////////////////////////////////////////////////////////
	// template <>
	// struct appendor
	// {
	// 	template <typename ...Ps>
	// 	auto operator () (Ps && ...ps)
	// 	{
	// 	}
	// };
	template <typename T>
	struct constructor
	{
		template <typename ...Ps>
		T operator () (Ps && ...ps)
		{
			return T{std::forward<Ps>(ps)...};
		}
	};
	struct negator
	{
		template <typename P>
		auto operator () (P && p) ->
			decltype(-std::forward<P>(p))
		{
			return -std::forward<P>(p);
		}
	};

	////////////////////////////////////////////////////////////////////////////
	//
	//  functions and whatnot
	//
	//////////////////////////////////////////////////////////////////
	template <typename T, std::size_t N, std::size_t ...Is, typename ...Ps>
	inline auto append_impl(const std::array<T, N> & array,
	                        mpl::index_sequence<Is...>,
	                        Ps && ...ps) ->
		decltype(std::array<T, N + sizeof...(Ps)>{std::get<Is>(array)...,
		                                          std::forward<Ps>(ps)...})
	{
		return std::array<T, N + sizeof...(Ps)>{std::get<Is>(array)...,
		                                        std::forward<Ps>(ps)...};
	}
	template <typename ...Ts, std::size_t ...Is, typename ...Ps>
	inline auto append_impl(const std::tuple<Ts...> & array,
	                        mpl::index_sequence<Is...>,
	                        Ps && ...ps) ->
		decltype(std::tuple<Ts..., mpl::decay_t<Ps>...>{std::get<Is>(array)...,
		                                                std::forward<Ps>(ps)...})
	{
		return std::tuple<Ts..., mpl::decay_t<Ps>...>{std::get<Is>(array)...,
		                                              std::forward<Ps>(ps)...};
	}
	template <typename Array, typename ...Ps>
	inline auto append(Array && array, Ps && ...ps) ->
		decltype(append_impl(std::forward<Array>(array),
		                     ext::make_tuple_sequence<Array>{},
		                     std::forward<Ps>(ps)...))
	{
		return append_impl(std::forward<Array>(array),
		                   ext::make_tuple_sequence<Array>{},
		                   std::forward<Ps>(ps)...);
	}

	template <typename Array>
	inline auto concat(Array && array) ->
		decltype(std::forward<Array>(array))
	{
		return std::forward<Array>(array);
	}
	template <typename Array1, typename Array2, std::size_t ...Is>
	inline auto concat_impl(Array1 && array1,
	                        Array2 && array2,
	                        mpl::index_sequence<Is...>) ->
		decltype(append(std::forward<Array1>(array1),
		                std::get<Is>(array2)...))
	{
		return append(std::forward<Array1>(array1),
		              std::get<Is>(array2)...);
	}
	template <typename Array1, typename Array2>
	inline auto concat(Array1 && array1, Array2 && array2) ->
		decltype(concat_impl(std::forward<Array1>(array1),
		                     std::forward<Array2>(array2),
		                     ext::make_tuple_sequence<Array2>{}))
	{
		return concat_impl(std::forward<Array1>(array1),
		                   std::forward<Array2>(array2),
		                   ext::make_tuple_sequence<Array2>{});
	}
	template <typename Array1, typename Array2, typename ...Arrays>
	inline auto concat(Array1 && array1, Array2 && array2, Arrays && ...arrays) ->
		decltype(concat(concat(std::forward<Array1>(array1), std::forward<Array2>(array2)),
		                std::forward<Arrays>(arrays)...))
	{
		return concat(concat(std::forward<Array1>(array1), std::forward<Array2>(array2)),
		              std::forward<Arrays>(arrays)...);
	}

	template <typename Array>
	inline auto flip(Array && array) ->
		decltype(map(std::forward<Array>(array),
		             negator{}))
	{
		return map(std::forward<Array>(array),
		           negator{});
	}

	struct Identity
	{
		template <typename P>
		constexpr decltype(auto) operator () (P && p)
		{
			return std::forward<P>(p);
		}
	};

	template <typename T>
	struct StaticCast
	{
		template <typename P>
		constexpr T operator () (P && p) { return static_cast<T>(std::forward<P>(p)); }
	};

	template <typename InputIt, typename T>
	constexpr std::ptrdiff_t index_of_impl(InputIt begin, InputIt it, InputIt end, const T & value)
	{
		return it == end || *it == value ? it - begin : index_of_impl(begin, ++it, end, value);
	}
	template <typename R, typename T>
	constexpr std::ptrdiff_t index_of(const R & range, const T & value)
	{
		using std::begin;
		using std::end;

		return index_of_impl(begin(range), begin(range), end(range), value);
	}

	template <typename T, typename F, typename I, I ...Is>
	constexpr auto inverse_table_impl(const T & table, F && f, mpl::integral_sequence<I, Is...>)
	{
		return std::array<mpl::remove_cvref_t<decltype(f(std::declval<std::ptrdiff_t>()))>, sizeof...(Is)>{{f(index_of(table, Is))...}};
	}
	template <std::size_t N, typename T>
	constexpr auto inverse_table(const T & table)
	{
		using value_type = mpl::remove_cvref_t<decltype(table[0])>;

		return inverse_table_impl(table, StaticCast<std::size_t>{}, mpl::make_integral_sequence<value_type, N>{});
	}
	template <std::size_t N, typename T, typename F>
	constexpr auto inverse_table(const T & table, F && f)
	{
		using value_type = mpl::remove_cvref_t<decltype(table[0])>;

		return inverse_table_impl(table, std::forward<F>(f), mpl::make_integral_sequence<value_type, N>{});
	}

	template <typename Array, std::size_t ...Is>
	inline auto shuffle(Array && array,
	                    mpl::index_sequence<Is...>) ->
		decltype(mpl::decay_t<Array>{{std::get<Is>(array)...}})
	{
		return mpl::decay_t<Array>{{std::get<Is>(array)...}};
	}

	template <std::size_t N, typename Array>
	inline auto take(Array && array) ->
		decltype(shuffle(std::forward<Array>(array), mpl::make_index_sequence<N>{}))
	{
		return shuffle(std::forward<Array>(array), mpl::make_index_sequence<N>{});
	}
	template <typename T, std::size_t R, std::size_t C>
	inline auto transpose(const std::array<T, R * C> & array) ->
		decltype(shuffle(array,
		                 mpl::make_transpose_sequence<R, C>{}))
	{
		return shuffle(array,
		               mpl::make_transpose_sequence<R, C>{});
	}

	template <typename T, std::size_t N, T Default>
	struct IndexCast
	{
		template <typename P>
		constexpr T operator () (P && p) const { return std::size_t(p) < N ? static_cast<T>(std::move(p)) : Default; }
	};

	template <std::size_t N, typename T = void, typename ...Pairs>
	constexpr decltype(auto) make_table(Pairs && ...pairs)
	{
		using value_type = mpl::common_type_unless_nonvoid<T, mpl::remove_cv_t<typename mpl::remove_cvref_t<Pairs>::second_type>...>;

		value_type values[N] = {};

		int expansion_hack[] = {(values[pairs.first] = std::move(pairs.second), 0)...};
		static_cast<void>(expansion_hack);

		return utl::to_array(std::move(values));
	}
}
