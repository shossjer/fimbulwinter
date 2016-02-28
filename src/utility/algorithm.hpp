
#ifndef UTILITY_ALGORITHM_HPP
#define UTILITY_ALGORITHM_HPP

#include "type_traits.hpp"

#include <array>

namespace utility
{
	template <typename T, std::size_t N, std::size_t ...Is>
	inline std::array<T, N> shuffle(const std::array<T, N> & array,
	                                mpl::index_sequence<Is...>)
	{
		return {array[Is]...};
	}
	template <typename T, std::size_t R, std::size_t C>
	inline std::array<T, R * C> transpose(const std::array<T, R * C> & array)
	{
		return shuffle(array, mpl::make_transpose_sequence<R, C>{});
	}
}

#endif /* UTILITY_ALGORITHM_HPP */
