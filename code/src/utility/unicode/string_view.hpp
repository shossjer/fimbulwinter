#pragma once

#include "utility/string_view.hpp"
#include "utility/unicode/encoding_traits.hpp"

namespace utility
{
	using string_units_utf8 = basic_string_view<boundary_unit_utf8>;
	using string_units_utf16 = basic_string_view<boundary_unit_utf16>;
	using string_units_utf32 = basic_string_view<boundary_unit_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using string_units_utfw = basic_string_view<boundary_unit_utfw>;
#endif

	using string_points_utf8 = basic_string_view<boundary_point_utf8>;
	using string_points_utf16 = basic_string_view<boundary_point_utf16>;
	using string_points_utf32 = basic_string_view<boundary_point_utf32>;
#if defined(_MSC_VER) && defined(_UNICODE)
	using string_points_utfw = basic_string_view<boundary_point_utfw>;
#endif
}
