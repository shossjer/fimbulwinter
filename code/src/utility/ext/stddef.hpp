#pragma once

#include <cstddef>

namespace ext
{
	using usize = std::size_t;
	using pdiff = std::ptrdiff_t;

	using ssize = pdiff;

	// Herb Sutter makes a very convincing argument for why an index
	// type is needed. We will follow his lead be adding one here.
	//
	// https://github.com/isocpp/CppCoreGuidelines/pull/1115
	using index = ssize;

	constexpr auto index_invalid = index(-1);
}
