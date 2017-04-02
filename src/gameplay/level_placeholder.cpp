
#include "level_placeholder.hpp"

#include <core/maths/algorithm.hpp>

#include <gameplay/characters.hpp>
#include <gameplay/assets.hpp>

#include <engine/Entity.hpp>
#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace gameplay
{
namespace level
{
	namespace
	{
		using Entity = engine::Entity;

		using transform_t = ::engine::transform_t;

		// the data needed when creating instances of an asset.
		// renderer meshes and physics shapes should already be added in respective module.
		struct asset_template_t
		{
			struct part_t
			{
				Entity id;
			};

			std::unordered_map<std::string, part_t> parts;

			const part_t & part(const std::string name) const
			{
				auto itr = parts.find(name);

				if (itr == parts.end())
				{
					debug_unreachable();
				}

				return itr->second;
			}

			//// renderer mesh entities
			//struct mesh_t
			//{
			//	engine::Entity id;
			//};

			//std::vector<mesh_t> meshes;

			//// physics shape entities
			//struct shape_t
			//{
			//	engine::Entity id;
			//};

			//std::vector<shape_t> shapes;

			//// relations

			//// render texture data..?
		};

		// contains all prev. loaded asset definitions.
		std::unordered_map<std::string, asset_template_t> assets;

		// represents model data in file
		// only used when loading a model file. common for all models.
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

		model_t load_model_data(const std::string & type)
		{
			// TODO: perform some check if the file exists
			const json jcontent = json::parse(std::ifstream("res/" + type + ".json"));

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

		const asset_template_t & load(const std::string & type)
		{
			// check if already exists
			auto itr = assets.find(type);

			if (itr != assets.end()) return itr->second;

			asset_template_t asset_template;

			// the asset has not previously been loaded
			const model_t model = load_model_data(type);

			for (const auto & part : model.parts)
			{
				// register renderer definition of asset
				{
					engine::graphics::renderer::asset_definition_t assetDef;

					for (const auto & mesh : part.render.shapes)
					{
						assetDef.meshs.emplace_back(
								engine::graphics::data::MeshC{
										mesh.vertices, // vertices
										mesh.indices, // triangles
										mesh.normals, // normals
										mesh.color }); // color);
					}
					const auto id = Entity::create();

					engine::graphics::renderer::add(id, assetDef);

					asset_template.parts.emplace(part.name, asset_template_t::part_t{id});
				}
				// register physics definition of asset
				{

				}
			}

			// create the asset
			const auto & r = assets.emplace(type, std::move(asset_template));

			// whats up with this syntax
			return r.first->second;
		}
	}


	engine::Entity load(const placeholder_t & placeholder, const std::string & type)
	{
		// TEMP
		if (type == "turret")
		{
			return Entity::INVALID;
		}
		else
		if (type == "beam")
		{
			return Entity::INVALID;
		}

		const auto & transform = placeholder.transform;

		// load the model (if not already loaded)
		const asset_template_t & assetTemplate = load(type);

		const auto id = Entity::create();

		if (type=="droid")
		{
			const auto headId = Entity::create();

			// create the object instance
			asset::droid_t assetInstance{id, transform, headId };

			// register new asset in renderer
			{
				engine::graphics::renderer::add(
						id,
						engine::graphics::renderer::asset_instance_t{
								assetTemplate.part("drive").id,
								transform.matrix() });

				engine::graphics::renderer::add(
						headId,
						engine::graphics::renderer::asset_instance_t{
								assetTemplate.part("head").id,
								transform.matrix() });
			}
			// register new asset in physics

			// register new asset in character
			//characters::post_add(assetInstance);
		}
		else
		{
			return Entity::INVALID;
		}

		return id;
	}
}
}
