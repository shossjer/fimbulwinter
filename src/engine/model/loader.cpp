
#include "data.hpp"
#include "loader.hpp"

#include <core/maths/algorithm.hpp>

#include <engine/Entity.hpp>
#include <engine/debug.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/defines.hpp>

#include <utility/json.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace engine
{
namespace model
{
	namespace
	{
		using Entity = engine::Entity;

		using transform_t = ::engine::transform_t;

		// contains all prev. loaded asset definitions.
		std::unordered_map<engine::Asset::value_type, asset_template_t> assets;

		std::unordered_map<engine::Asset::value_type, bool> assetsBin;


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

			struct location_t
			{
				std::string name;
				transform_t transform;
			};

			std::vector<location_t> locations;

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

		model_t load_model_data(const std::string & modelName)
		{
			std::ifstream ifile("res/" + modelName + ".json");

			if (!ifile)
			{
				throw std::runtime_error("File does not exist for model: " + modelName);
			}
			// TODO: perform some check if the file exists
			const json jcontent = json::parse(ifile);

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

			const auto & jlocations = jcontent["locations"];

			// read the relations (assume joints only for now)
			for (const auto & jlocation : jlocations)
			{
				const std::string name = jlocation["name"];

				model.locations.emplace_back(model_t::location_t{ name, load_transform(jlocation) });
			}

			return model;
		}
	}

	const asset_template_t & load(const engine::Asset asset, const std::string & modelName)
	{
		auto itr = assets.find(asset);

		// check if already exists
		if (itr != assets.end()) return itr->second;

		// the asset has not previously been loaded
		asset_template_t asset_template;

		const model_t model = load_model_data(modelName);

		for (const auto & part : model.parts)
		{
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

				engine::graphics::renderer::add(asset, assetDef);
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
							engine::physics::ShapeData::Geometry::Box{ shape.w, shape.h, shape.d*.5f } };
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

				//engine::physics::asset_definition_t assetDef{ engine::physics::ActorData::Behaviour::CHARACTER, shapes };

				//engine::physics::add(id, assetDef);
			}

			// save the defined part to be used when creating instances.
			asset_template.parts.emplace(part.name, asset_template_t::part_t{ asset });
		}

		for (const auto & joint : model.joints)
		{
			asset_template.joints.emplace(joint.name, asset_template_t::joint_t{ joint.transform });
		}

		for (const auto & location : model.locations)
		{
			asset_template.locations.emplace(location.name, asset_template_t::location_t{ location.transform });
		}

		// create the asset
		const auto & r = assets.emplace(asset, std::move(asset_template));

		// whats up with this syntax
		return r.first->second;
	}

	uint16_t read_count(std::ifstream & stream)
	{
		uint16_t count;
		stream.read(reinterpret_cast<char *>(&count), sizeof(uint16_t));
		return count;
	}

	template <std::size_t N>
	void read_string(std::ifstream & stream, char(&buffer)[N])
	{
		uint16_t len; // excluding null character
		stream.read(reinterpret_cast<char *>(&len), sizeof(uint16_t));
		debug_assert(len < N);

		stream.read(buffer, len);
		buffer[len] = '\0';
	}
	void read_string(std::ifstream & stream, std::string & str)
	{
		char buffer[64]; // arbitrary
		read_string(stream, buffer);
		str = buffer;
	}

	template <class T, std::size_t N>
	void read(std::ifstream & stream, core::container::Buffer & buffer)
	{
		const uint16_t count = read_count(stream);
		buffer.resize<T>(N * count);
		stream.read(reinterpret_cast<char *>(buffer.data()), buffer.size());
	}

	void read_weights(std::ifstream & stream, mesh_t & mesh)
	{
		const uint16_t weights = read_count(stream);

		mesh.weights.resize(weights);
		for (auto && weight : mesh.weights)
		{
			const uint16_t ngroups = read_count(stream);

			debug_assert(ngroups > 0);

			// read the first weight value
			weight.index = read_count(stream);
			stream.read(reinterpret_cast<char *>(&weight.value), sizeof(float));

			// skipp remaining ones if any for now
			for (auto i = 1; i < ngroups; i++)
			{
				read_count(stream);
				float value;
				stream.read(reinterpret_cast<char *>(&value), sizeof(float));
			}
		}
	}

	void read_matrix(std::ifstream & stream, core::maths::Matrix4x4f & matrix)
	{
		core::maths::Matrix4x4f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		matrix.set_aligned(buffer);
	}

	bool load_binary(const std::string filename)
	{
		auto itr = assetsBin.find(engine::Asset{filename});

		if (itr != assetsBin.end()) return itr->second;

		std::ifstream stream("res/" + filename + ".msh", std::ifstream::binary);

		if (!stream)
		{
			assetsBin.emplace(engine::Asset{ filename }, false);
			return false;
		}

		assetsBin.emplace(engine::Asset{ filename }, true);

		engine::Asset asset{ filename };
		engine::model::mesh_t mesh;

		{
			std::string name;
			read_string(stream, name);
		//	asset = name;
			debug_printline(0xffffffff, "mesh name: ", name);
		}

		read_matrix(stream, mesh.matrix);

		read<float, 3>(stream, mesh.xyz);
		debug_printline(0xffffffff, "mesh vertices: ", mesh.xyz.count() / 3);

		read<float, 2>(stream, mesh.uv);
		debug_printline(0xffffffff, "mesh uv's: ", mesh.uv.count() / 2);

		read<float, 3>(stream, mesh.normals);
		debug_printline(0xffffffff, "mesh normals: ", mesh.normals.count() / 3);

		read_weights(stream, mesh);
		debug_printline(0xffffffff, "mesh weights: ", mesh.weights.size());

		read<unsigned int, 3>(stream, mesh.triangles);
		debug_printline(0xffffffff, "mesh triangles: ", mesh.triangles.count() / 3);

		// post mesh to renderer
		engine::graphics::renderer::add(asset, std::move(mesh));

		return true;
	}
}
}
