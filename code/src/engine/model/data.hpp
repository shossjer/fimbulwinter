
#ifndef ENGINE_MODEL_DATA_HPP
#define ENGINE_MODEL_DATA_HPP

#include <core/container/Buffer.hpp>
#include "core/serialization.hpp"

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

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("index"), &weight_t::index),
				std::make_pair(utility::string_view("value"), &weight_t::value)
				);
		}
	};
	struct mesh_t
	{
		core::maths::Matrix4x4f matrix;
		core::container::Buffer xyz;
		core::container::Buffer uv;
		std::vector<weight_t> weights;
		core::container::Buffer normals;
		core::container::Buffer triangles;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("matrix"), &mesh_t::matrix),
				std::make_pair(utility::string_view("vertices"), &mesh_t::xyz),
				std::make_pair(utility::string_view("uvs"), &mesh_t::uv),
				std::make_pair(utility::string_view("weights"), &mesh_t::weights),
				std::make_pair(utility::string_view("normals"), &mesh_t::normals),
				std::make_pair(utility::string_view("triangles"), &mesh_t::triangles)
				);
		}
	};
}
}

#endif // ENGINE_MODEL_DATA_HPP
