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
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("matrix"), &transform_t::matrix)
				);
		}
	};
}
