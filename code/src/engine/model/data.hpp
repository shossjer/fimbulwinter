#pragma once

#include "core/container/Buffer.hpp"
#include "core/serialization.hpp"

#include "engine/common.hpp"

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
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("index"), &weight_t::index),
				std::make_pair(ful::cstr_utf8("value"), &weight_t::value)
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
			return utility::make_lookup_table<ful::view_utf8>(
				std::make_pair(ful::cstr_utf8("matrix"), &mesh_t::matrix),
				std::make_pair(ful::cstr_utf8("vertices"), &mesh_t::xyz),
				std::make_pair(ful::cstr_utf8("uvs"), &mesh_t::uv),
				std::make_pair(ful::cstr_utf8("weights"), &mesh_t::weights),
				std::make_pair(ful::cstr_utf8("normals"), &mesh_t::normals),
				std::make_pair(ful::cstr_utf8("triangles"), &mesh_t::triangles)
				);
		}
	};
}
}
