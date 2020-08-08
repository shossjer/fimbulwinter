#pragma once

#include "core/maths/util.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Vector.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/serialization.hpp"

namespace engine
{
	struct transform_t
	{
		core::maths::Vector3f pos;
		core::maths::Quaternionf quat;

		core::maths::Matrix4x4f matrix() const
		{
			return
				make_translation_matrix(pos) *
				make_matrix(quat);
		}

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_units_utf8("pos"), &transform_t::pos),
				std::make_pair(utility::string_units_utf8("quat"), &transform_t::quat)
				);
		}
	};
}
