#pragma once

#include "utility/functional/common.hpp"

namespace fun
{
	const auto equal_to = detail::function([](auto && left, auto && right)
	{
		return detail::eval(
			[](auto && left, auto && right) -> decltype(auto)
			{
				return std::forward<decltype(left)>(left) == std::forward<decltype(right)>(right);
			},
			std::forward<decltype(left)>(left), std::forward<decltype(right)>(right));
	});
}
