#pragma once

#include <utility>

namespace utility
{
	template <typename T>
	struct if_name_is_impl
	{
		using this_type = if_name_is_impl<T>;

		T t;

		template <typename U>
		friend bool operator == (U && u, this_type a) { return std::forward<U>(u).name == std::forward<T>(a.t); }
	};

	template <typename T>
	auto if_name_is(T && t)
	{
		return if_name_is_impl<T &&>{std::forward<T>(t)};
	}
}
