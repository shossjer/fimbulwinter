
#ifndef ENGINE_COMMON_HPP
#define ENGINE_COMMON_HPP

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
				std::make_pair(utility::string_view("pos"), &transform_t::pos),
				std::make_pair(utility::string_view("quat"), &transform_t::quat)
				);
		}
	};
}

#endif /* ENGINE_COMMON_HPP */
