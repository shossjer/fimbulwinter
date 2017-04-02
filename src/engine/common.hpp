
#ifndef ENGINE_COMMON_HPP
#define ENGINE_COMMON_HPP

#include <engine/Entity.hpp>

#include <core/maths/util.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/Quaternion.hpp>

#include <vector>

using Matrix4x4f = core::maths::Matrix4x4f;
using Vector3f = core::maths::Vector3f;
using Quaternionf = core::maths::Quaternionf;

namespace engine
{
	struct transform_t
	{
		Vector3f pos;
		Quaternionf quat;

		Matrix4x4f matrix() const
		{
			return
				make_translation_matrix(core::maths::Vector3f{ 15.f, 2.f, 0.f }) *
				make_matrix(core::maths::Quaternionf{ 1.f, 0.f, 0.f, 0.f });
		}
	};
}

#endif // ENGINE_COMMON_HPP
