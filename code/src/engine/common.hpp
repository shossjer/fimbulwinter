#pragma once

#include "core/maths/Matrix.hpp"
#include "core/serialization.hpp"

namespace engine
{
	struct transform_t
	{
		core::maths::Matrix4x4f matrix;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("matrix"), &transform_t::matrix)
				);
		}
	};
}
