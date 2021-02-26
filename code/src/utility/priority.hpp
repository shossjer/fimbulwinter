#pragma once

namespace ext
{
	template <int N>
	struct priority : priority<(N + 1)> {};

	template <>
	struct priority<5> {};

	using priority_highest = const priority<0> *;
	using priority_high = const priority<1> *;
	using priority_normal = const priority<2> *;
	using priority_low = const priority<3> *;
	using priority_lowest = const priority<4> *;
	using priority_default = const priority<5> *;

	constexpr const priority_highest resolve_priority = nullptr;
}
