
#ifndef UTILITY_ALGORITHM_HPP
#define UTILITY_ALGORITHM_HPP

#include "type_traits.hpp"

#include <algorithm>
#include <array>
#include <tuple>

namespace utl
{
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
		                 mpl::make_array_sequence<mpl::decay_t<Array>>{},
		                 std::forward<F>(f)))
	{
		return map_for(std::forward<Array>(array),
		               mpl::make_array_sequence<mpl::decay_t<Array>>{},
		               std::forward<F>(f));
	}

	template <typename Array, std::size_t ...Is, typename F>
	inline auto unpack_for(Array && array,
	                       mpl::index_sequence<Is...>,
	                       F && f) ->
		decltype(std::forward<F>(f)(std::get<Is>(array)...))
	{
		return std::forward<F>(f)(std::get<Is>(array)...);
	}
	template <typename Array, typename F>
	inline auto unpack(Array && array,
	                   F && f) ->
		decltype(unpack_for(std::forward<Array>(array),
		                    mpl::make_array_sequence<mpl::decay_t<Array>>{},
		                    std::forward<F>(f)))
	{
		return unpack_for(std::forward<Array>(array),
		                  mpl::make_array_sequence<mpl::decay_t<Array>>{},
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
		                     mpl::make_array_sequence<mpl::decay_t<Array>>{},
		                     std::forward<Ps>(ps)...))
	{
		return append_impl(std::forward<Array>(array),
		                   mpl::make_array_sequence<mpl::decay_t<Array>>{},
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
		                     mpl::make_array_sequence<mpl::decay_t<Array2>>{}))
	{
		return concat_impl(std::forward<Array1>(array1),
		                   std::forward<Array2>(array2),
		                   mpl::make_array_sequence<mpl::decay_t<Array2>>{});
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
}

#endif /* UTILITY_ALGORITHM_HPP */
