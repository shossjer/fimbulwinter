
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

		using render_instance_t = engine::graphics::renderer::asset_instance_t;
		using physic_instance_t = engine::physics::asset_instance_t;

		// the data needed when creating instances of an asset.
		// renderer meshes and physics shapes should already be added in respective module.
		struct asset_template_t
		{
			struct part_t
			{
				Entity id;
			};

			std::unordered_map<std::string, part_t> parts;

			struct joint_t
			{
				transform_t transform;
			};

			std::unordered_map<std::string, joint_t> joints;

			template<typename T>
			const T & get(
					const std::unordered_map<std::string, T> & items,
					const std::string name) const
			{
				auto itr = items.find(name);

				if (itr == items.end())
				{
					debug_unreachable();
				}

				return itr->second;
			}

			const part_t & part(const std::string name) const
			{
				return get<part_t>(this->parts, name);
			}

			const joint_t & joint(const std::string name) const
			{
				return get<joint_t>(this->joints, name);
			}
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
						enum class Type
						{
							MESH
						}	type;
						engine::graphics::data::Color color;
						core::container::Buffer vertices;
						core::container::Buffer indices;
						core::container::Buffer normals;
					};

					std::vector<shape_t> shapes;
				};

				struct physic_t
				{
					struct shape_t
					{
						transform_t transform;

						enum class Type
						{
							BOX,
							MESH
						}	type;

						std::vector<Vector3f> points;
						float w, h, d;
					};

					std::vector<shape_t> shapes;
				};

				std::string name;
				render_t render;
				physic_t physic;
			};

			std::vector<part_t> parts;

			struct joint_t
			{
				std::string name;
				transform_t transform;
			};

			std::vector<joint_t> joints;

			template<typename T>
			const T & get(const std::vector<T> & items, const std::string & name) const
			{
				for (const auto i : items)
				{
					if (i.name == name) return i;
				}

				debug_unreachable();
			}

			const part_t & part(const std::string & name) const
			{
				return get<part_t>(this->parts, name);
			}

			const joint_t & joint(const std::string & name) const
			{
				return get<joint_t>(this->joints, name);
			}
		};

		transform_t load_transform(const json & jparent)
		{
			const auto & jtransform = jparent["transform"];
			const auto & jp = jtransform["pos"];
			const auto & jq = jtransform["quat"];

			return transform_t{
					Vector3f{ jp[0], jp[1], jp[2] },
					Quaternionf{ jq[0], jq[1], jq[2], jq[3] } };
		}

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

						const std::string type = jshape["type"];

						if (type == "MESH")
						{
							shape.type = model_t::part_t::render_t::shape_t::Type::MESH;
						}
						else
						{
							debug_unreachable();
						}

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

				model_t::part_t::physic_t physic{};
				{
					const json & jphysic = jpart["physic"];
					const json & jshapes = jphysic["shapes"];

					for (const json & jshape : jshapes)
					{
						model_t::part_t::physic_t::shape_t shape;

						shape.transform = load_transform(jshape);

						const std::string type = jshape["type"];

						if (type == "BOX")
						{
							shape.type = model_t::part_t::physic_t::shape_t::Type::BOX;

							const json & jscale = jshape["scale"];

							//shape.points.emplace_back(Vector3f{jscale[0], jscale[1], jscale[2]});
							shape.w = jscale[0];
							shape.h = jscale[1];
							shape.d = jscale[2];
						}
						else
						if (type == "MESH")
						{
							shape.type = model_t::part_t::physic_t::shape_t::Type::MESH;

							// TODO: read all points in the mesh
						}
						else
						{
							debug_unreachable();
						}

						physic.shapes.push_back(shape);
					}
				}

				model.parts.emplace_back(model_t::part_t { name, render, physic });
			}

			const auto & jrelations = jcontent["relations"];

			// read the relations (assume joints only for now)
			for (const auto & jrelation : jrelations)
			{
				const std::string name = jrelation["name"];
				const auto & jjoint = jrelation["joint"];

				model.joints.emplace_back(model_t::joint_t{ name, load_transform(jjoint) });
			}

			return model;
		}

		const asset_template_t & load(const std::string & type)
		{
			auto itr = assets.find(type);

			// check if already exists
			if (itr != assets.end()) return itr->second;

			// the asset has not previously been loaded
			asset_template_t asset_template;

			const model_t model = load_model_data(type);

			for (const auto & part : model.parts)
			{
				// the id of the model part definition
				const auto id = Entity::create();

				if (!part.render.shapes.empty())
				{
					// register renderer definition of asset
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

					engine::graphics::renderer::add(id, assetDef);
				}

				if (!part.physic.shapes.empty())
				{
					// register physics definition of asset

					std::vector<engine::physics::ShapeData> shapes;

					for (const auto & shape : part.physic.shapes)
					{
						engine::physics::ShapeData::Type type;
						engine::physics::ShapeData::Geometry geometry;

						switch (shape.type)
						{
						case model_t::part_t::physic_t::shape_t::Type::BOX:

							type = engine::physics::ShapeData::Type::BOX;
							geometry = engine::physics::ShapeData::Geometry{
									engine::physics::ShapeData::Geometry::Box{shape.w, shape.h, shape.d*.5f } };
							break;

						default:

							debug_unreachable();
						}

						// TODO: somehow load solidity and material
						shapes.emplace_back(engine::physics::ShapeData{
								type,
								engine::physics::Material::WOOD,
								0.25f,
								shape.transform,
								geometry });
					}

					engine::physics::asset_definition_t assetDef{ engine::physics::ActorData::Behaviour::CHARACTER, shapes };

					engine::physics::add(id, assetDef);
				}

				// save the defined part to be used when creating instances.
				asset_template.parts.emplace(part.name, asset_template_t::part_t{ id });
			}

			for (const auto & joint : model.joints)
			{
				asset_template.joints.emplace(joint.name, asset_template_t::joint_t{ joint.transform });
			}

			// create the asset
			const auto & r = assets.emplace(type, std::move(asset_template));

			// whats up with this syntax
			return r.first->second;
		}
	}


	engine::Entity load(
			const placeholder_t & placeholder,
			const std::string & type,
			const engine::physics::ActorData::Behaviour behaviour)
	{
		const auto & transform = placeholder.transform;

		// load the model (if not already loaded)
		const asset_template_t & assetTemplate = load(type);

		const auto id = Entity::create();

		if (type=="droid")
		{
			const auto headId = Entity::create();

			// create the object instance
			asset::droid_t assetInstance{id, transform, headId, Entity::create() };

			const auto & drivePart = assetTemplate.part("drive");
			const auto & headPart = assetTemplate.part("head");

			// register new asset in renderer
			engine::graphics::renderer::add(
					id,
					render_instance_t{
							drivePart.id,
							transform.matrix() });
			engine::graphics::renderer::add(
					headId,
					render_instance_t{
							headPart.id,
							transform.matrix() });

			// register new asset in physics
			engine::physics::add(
					id,
					physic_instance_t{
							drivePart.id,
							placeholder.transform,
							engine::physics::ActorData::Type::DYNAMIC });
			engine::physics::add(
					headId,
					physic_instance_t{
							headPart.id,
							placeholder.transform,
							engine::physics::ActorData::Type::DYNAMIC });

			const auto & jointDef = assetTemplate.joint("drive");

			// create joint between drive and head
			engine::physics::post_joint(
					engine::physics::joint_t {
							assetInstance.jointId,
							engine::physics::joint_t::Type::HINGE,
							assetInstance.id,
							assetInstance.headId,
							transform_t {
									jointDef.transform.pos,
									jointDef.transform.quat *core::maths::rotation_of(Vector3f{0.f, 1.f, 0.f}) },
							transform_t {
									Vector3f{0.f, 0.f, 0.f},
									Quaternionf{1.f, 0.f, 0.f, 0.f}* core::maths::rotation_of(Vector3f{0.f, 1.f, 0.f})} });

			// register new asset in character
			characters::post_add(assetInstance);
		}
		else
		{
			return Entity::INVALID;
		}

		return id;
	}
}
}
