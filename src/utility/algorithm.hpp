
#ifndef UTILITY_ALGORITHM_HPP
#define UTILITY_ALGORITHM_HPP

#include "type_traits.hpp"

#include <array>

namespace utility
{
	template <typename T, std::size_t N, typename ...Ps, std::size_t ...Is>
	inline std::array<T, N + sizeof...(Ps)> append_impl(const std::array<T, N> & array,
	                                                    Ps && ...ps,
	                                                    mpl::index_sequence<Is...>)
	{
		return {std::get<Is>(array)..., std::forward<Ps>(ps)...};
	}
	template <typename T, std::size_t N, typename ...Ps>
	inline std::array<T, N + sizeof...(Ps)> append(const std::array<T, N> & array,
	                                               Ps && ...ps)
	{
		return append_impl(array, std::forward<Ps>(ps)..., mpl::make_index_sequence<N>{});
	}

	template <typename T>
	struct constructor
	{
		template <typename ...Ps>
		T operator () (Ps && ...ps)
		{
			return T{std::forward<Ps>(ps)...};
		}
	};

	template <typename T, std::size_t N, std::size_t ...Is>
	inline std::array<T, N> flip_for(const std::array<T, N> & array,
	                                 mpl::index_sequence<Is...>)
	{
		return {-std::get<Is>(array)...};
	}
	template <typename T, std::size_t N>
	inline std::array<T, N> flip(const std::array<T, N> & array)
	{
		return flip_for(array, mpl::make_index_sequence<N>{});
	}

	template <typename T, std::size_t N, std::size_t ...Is>
	inline std::array<T, N> shuffle(const std::array<T, N> & array,
	                                mpl::index_sequence<Is...>)
	{
		return {std::get<Is>(array)...};
	}

	template <std::size_t N, typename Array>
	inline auto take(Array && array) ->
		decltype(shuffle(std::forward<Array>(array), mpl::make_index_sequence<N>{}))
	{
		return shuffle(std::forward<Array>(array), mpl::make_index_sequence<N>{});
	}
	template <typename T, std::size_t R, std::size_t C>
	inline std::array<T, R * C> transpose(const std::array<T, R * C> & array)
	{
		return shuffle(array, mpl::make_transpose_sequence<R, C>{});
	}

	template <typename Array, typename F, std::size_t ...Is>
	inline auto unpack(Array && array, F && f, mpl::index_sequence<Is...>) ->
		decltype(std::forward<F>(f)(std::get<Is>(array)...))
	{
		return std::forward<F>(f)(std::get<Is>(array)...);
	}
	template <typename Array, typename F>
	inline auto unpack(Array && array, F && f) ->
		decltype(unpack(std::forward<Array>(array),
		                std::forward<F>(f),
		                mpl::make_array_sequence<mpl::decay_t<Array>>{}))
	{
		return unpack(std::forward<Array>(array),
		              std::forward<F>(f),
		              mpl::make_array_sequence<mpl::decay_t<Array>>{});
	}
}

#endif /* UTILITY_ALGORITHM_HPP */
