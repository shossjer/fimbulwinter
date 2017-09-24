
#ifndef ENGINE_MODEL_DATA_HPP
#define ENGINE_MODEL_DATA_HPP

#include <core/container/Buffer.hpp>

#include <engine/common.hpp>

#include <vector>

namespace engine
{
namespace model
{
	struct weight_t
	{
		uint16_t index;
		float value;
	};
	struct mesh_t
	{
		core::maths::Matrix4x4f matrix;
		core::container::Buffer xyz;
		core::container::Buffer uv;
		std::vector<weight_t> weights;
		core::container::Buffer normals;
		core::container::Buffer triangles;
	};
}
}

#endif // ENGINE_MODEL_DATA_HPP