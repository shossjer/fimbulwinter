#pragma once

#include "utility/functional/common.hpp"

namespace fun
{
	const auto first = detail::function([](auto && pair) -> decltype(auto) { return pair.first; });

	template <std::size_t I>
	const auto get = detail::function([](auto && tuple) -> decltype(auto)
	{
		return std::get<I>(std::forward<decltype(tuple)>(tuple));
	});
}
