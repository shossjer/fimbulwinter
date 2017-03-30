
#include "level_placeholder.hpp"

#include <core/maths/algorithm.hpp>

#include <gameplay/characters.hpp>

#include <engine/Entity.hpp>
#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>

#include <fstream>
#include <sstream>

namespace
{
	using namespace gameplay;
	using namespace gameplay::level;

	using Entity = engine::Entity;

	using transform_t = ::engine::transform_t;

	struct model_t
	{
		struct part_t
		{
			struct render_t
			{
				struct shape_t
				{
					std::string type;
					engine::graphics::data::Color color;
					core::container::Buffer vertices;
					core::container::Buffer indices;
					core::container::Buffer normals;
				};

				std::vector<shape_t> shapes;
			};

			std::string name;
			render_t render;
		};

		std::vector<part_t> parts;

		const part_t & part(const std::string & name) const
		{
			for(const auto i : this->parts)
			{
				if (i.name==name) return i;
			}

			debug_unreachable();
		}
	};

	model_t load_model_data(const json & jcontent)
	{
		model_t model {};

		const auto & jparts = jcontent["parts"];

		for(const auto & jpart : jparts)
		{
			const std::string name = jpart["name"];
			model_t::part_t::render_t render {};
			{
				const json & jrender = jpart["render"];
				const json & jshapes = jrender["shapes"];

				for (const json & jshape:jshapes)
				{
					model_t::part_t::render_t::shape_t shape;

					shape.type = jshape["type"];

					const json & jcolor = jshape["color"];
					const uint32_t r = ((float) jcolor["r"])*255;
					const uint32_t g = ((float) jcolor["g"])*255;
					const uint32_t b = ((float) jcolor["b"])*255;
					shape.color = r + (g<<8) + (b<<16) + (0xff<<24);

					const json & jindices = jshape["indices"];
					const json & jnormals = jshape["normals"];
					const json & jvertices = jshape["vertices"];
					shape.indices.resize<unsigned int>(jindices.size());
					std::copy(jindices.begin(), jindices.end(), shape.indices.data_as<unsigned int>());

					shape.normals.resize<float>(jnormals.size());
					std::copy(jnormals.begin(), jnormals.end(), shape.normals.data_as<float>());

					shape.vertices.resize<float>(jvertices.size());
					std::copy(jvertices.begin(), jvertices.end(), shape.vertices.data_as<float>());

					render.shapes.push_back(shape);
				}
			}

			model.parts.emplace_back(model_t::part_t {name, render});
		}

		return model;
	}

	void load_droid()
	{
		//  load model data
		const model_t model = load_model_data(json::parse(std::ifstream("res/droid.json")));

		debug_printline(0xffffffff, "Droid parts: %d", model.parts.size());
		for (auto part:model.parts)
		{
			debug_printline(0xffffffff, "part: %s", part.name);

			for (auto shape:part.render.shapes)
			{
				engine::Entity id = engine::Entity::create();

				engine::graphics::renderer::add(id, engine::graphics::data::MeshC {
					shape.vertices, // vertices
					shape.indices, // triangles
					shape.normals, // normals
					shape.color}); // color

				const core::maths::Matrix4x4f matrix =
					make_translation_matrix(core::maths::Vector3f {15.f, 2.f, 0.f}) *
					make_matrix(core::maths::Quaternionf {1.f, 0.f, 0.f, 0.f});

				engine::graphics::renderer::update(id, engine::graphics::data::ModelviewMatrix {matrix});
			}
		}
	}
}

namespace gameplay
{
namespace level
{
	void load(const placeholder_t & placeholder, const std::string & type)
	{
		if (type=="droid")
		{
			load_droid();
		}
		else
		{
			debug_printline(0xffffffff, "Warning: Placeholder is unknown: ", placeholder.name);
		}
	}
}
}
