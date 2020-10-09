#pragma once

#include "utility/algorithm/find.hpp"
#include "utility/functional/common.hpp"

namespace fun
{
	const auto contains = detail::function([](auto && range, auto && value)
	{
		return detail::eval(
			[](auto && range, auto && value)
			{
				return ext::contains(std::forward<decltype(range)>(range), std::forward<decltype(value)>(value));
			},
			std::forward<decltype(range)>(range), std::forward<decltype(value)>(value));
	});
}
