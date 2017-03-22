
#include "level.hpp"

#include "level_placeholder.hpp"

#include <gameplay/characters.hpp>

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>
#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>
#include <utility/string.hpp>

#include <fstream>
#include <vector>
#include <unordered_map>

namespace
{
	using namespace gameplay::level;

	struct box_t
	{
		core::maths::Vector3f translation;
		core::maths::Quaternionf rotation;
		core::maths::Vector3f scalation;
	};
	struct box2_t
	{
		core::maths::Matrix4x4f matrix; // either (matrix) or (translation and rotation) is enough
		core::maths::Vector3f translation;
		core::maths::Quaternionf rotation;
		float dimensions[3];
	};

	struct player_start_t
	{
		core::maths::Matrix4x4f matrix;
	};
	struct trigger_timer_t
	{
		box2_t start;
		box2_t stop;
	};

	struct static_t
	{
		std::string name;
		box_t box;
	};
	struct dynamic_t
	{
		std::string name;
		box2_t box;
	};
	struct platform_t
	{
		std::string name;
		box_t box;
		engine::animation::object animation;
	};

	struct trigger_multiple_t
	{
		std::string name;
		box2_t box;
	};

	struct mesh_t
	{
		std::string name;
		core::maths::Matrix4x4f matrix;
		std::vector<float> vertices;
		std::vector<uint16_t> triangles;
		std::vector<float> normals;

		mesh_t() {}
	};

	struct level_t
	{
		player_start_t player_start;
		trigger_timer_t trigger_timer;

		std::vector<static_t> statics;
		std::vector<dynamic_t> dynamics;
		std::vector<platform_t> platforms;

		std::vector<trigger_multiple_t> trigger_multiples;

		std::vector<mesh_t> meshes;

		std::vector<placeholder_t> placeholders;
	};

	void read_count(std::ifstream & stream, uint16_t & count)
	{
		stream.read(reinterpret_cast<char *>(& count), sizeof(uint16_t));
	}
	void read_length(std::ifstream & stream, int32_t & length)
	{
		stream.read(reinterpret_cast<char *>(& length), sizeof(int32_t));
	}
	void read_matrix(std::ifstream & stream, core::maths::Matrix4x4f & matrix)
	{
		core::maths::Matrix4x4f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		matrix.set_aligned(buffer);
	}
	void read_quaternion(std::ifstream & stream, core::maths::Quaternionf & quat)
	{
		core::maths::Quaternionf::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		quat.set_aligned(buffer);
	}
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (&buffer)[N])
	{
		uint16_t len; // excluding null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
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
	void read_vector(std::ifstream & stream, float (&buffer)[3])
	{
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
	}
	void read_vector(std::ifstream & stream, core::maths::Vector3f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set_aligned(buffer);
	}

	void read_box(std::ifstream & ifile, box_t & box)
	{
		read_vector(ifile, box.translation);
		read_quaternion(ifile, box.rotation);
		read_vector(ifile, box.scalation);
	}
	void read_box2(std::ifstream & ifile, box2_t & box)
	{
		read_matrix(ifile, box.matrix);
		read_vector(ifile, box.translation);
		read_quaternion(ifile, box.rotation);
		read_vector(ifile, box.dimensions);
	}

	void read_player_start(std::ifstream & ifile, player_start_t & player_start)
	{
		read_matrix(ifile, player_start.matrix);
	}
	void read_trigger_timer(std::ifstream & ifile, trigger_timer_t & trigger_timer)
	{
		read_box2(ifile, trigger_timer.start);
		read_box2(ifile, trigger_timer.stop);
	}

	void read_statics(std::ifstream & ifile, std::vector<static_t> & statics)
	{
		uint16_t nstatics;
		read_count(ifile, nstatics);

		statics.resize(nstatics);
		for (auto & stat : statics)
		{
			read_string(ifile, stat.name);
			read_box(ifile, stat.box);
		}
	}
	void read_dynamics(std::ifstream & ifile, std::vector<dynamic_t> & dynamics)
	{
		uint16_t ndynamics;
		read_count(ifile, ndynamics);

		dynamics.resize(ndynamics);
		for (auto & dynamic : dynamics)
		{
			read_string(ifile, dynamic.name);
			read_box2(ifile, dynamic.box);
		}
	}
	void read_platforms(std::ifstream & ifile, std::vector<platform_t> & platforms)
	{
		uint16_t nplatforms;
		read_count(ifile, nplatforms);

		platforms.resize(nplatforms);
		for (auto & platform : platforms)
		{
			read_string(ifile, platform.name);
			read_box(ifile, platform.box);

			platform.animation.name = platform.name;

			uint16_t nactions;
			read_count(ifile, nactions);
			debug_printline(0xffffffff, platform.name, " have ", nactions, " actions:");

			platform.animation.actions.resize(nactions);
			for (auto & action : platform.animation.actions)
			{
				read_string(ifile, action.name);
				debug_printline(0xffffffff, "  ", action.name);
				int length;
				read_length(ifile, length);

				action.keys.resize(length + 1);
				for (auto & key : action.keys)
				{
					read_vector(ifile, key.translation);
					read_quaternion(ifile, key.rotation);
				}
			}
		}
	}

	void read_trigger_multiples(std::ifstream & ifile, std::vector<trigger_multiple_t> & trigger_multiples)
	{
		uint16_t ntrigger_multiples;
		read_count(ifile, ntrigger_multiples);

		trigger_multiples.resize(ntrigger_multiples);
		for (auto & trigger_multiple : trigger_multiples)
		{
			read_string(ifile, trigger_multiple.name);
			read_box2(ifile, trigger_multiple.box);
		}
	}

	void read_meshes(std::ifstream & ifile, std::vector<mesh_t> & meshes)
	{
		uint16_t nmeshes;
		read_count(ifile, nmeshes);

		meshes.resize(nmeshes);
		for (auto & mesh : meshes)
		{
			read_string(ifile, mesh.name);
			read_matrix(ifile, mesh.matrix);

			uint16_t nvertices;
			read_count(ifile, nvertices);

			mesh.vertices.resize(int(nvertices) * 3);
			ifile.read(reinterpret_cast<char *>(mesh.vertices.data()), sizeof(float) * mesh.vertices.size());

			uint16_t nnormals;
			read_count(ifile, nnormals);
			debug_assert(nvertices == nnormals);

			mesh.normals.resize(int(nnormals) * 3);
			ifile.read(reinterpret_cast<char *>(mesh.normals.data()), sizeof(float) * mesh.normals.size());

			uint16_t ntriangles;
			read_count(ifile, ntriangles);

			mesh.triangles.resize(int(ntriangles) * 3);
			ifile.read(reinterpret_cast<char *>(mesh.triangles.data()), sizeof(uint16_t) * mesh.triangles.size());
		}
	}

	void read_placeholders(std::ifstream & ifile, std::vector<placeholder_t> & placeholders)
	{
		uint16_t nplaceholders;
		read_count(ifile, nplaceholders);

		placeholders.resize(nplaceholders);
		for (auto & placeholder : placeholders)
		{
			read_string(ifile, placeholder.name);
			read_vector(ifile, placeholder.pos);
			read_quaternion(ifile, placeholder.quat);
			read_vector(ifile, placeholder.scale);
		}
	}

	void read_level(std::ifstream & ifile, level_t & level)
	{
		read_player_start(ifile, level.player_start);
		read_trigger_timer(ifile, level.trigger_timer);

		read_statics(ifile, level.statics);
		read_dynamics(ifile, level.dynamics);
		read_platforms(ifile, level.platforms);

		read_trigger_multiples(ifile, level.trigger_multiples);

		read_meshes(ifile, level.meshes);

		read_placeholders(ifile, level.placeholders);
	}

	std::vector<engine::Entity> entities;
}

namespace gameplay
{
	namespace level
	{
		void create(const std::string & filename)
		{
			level_t level;
			{
				std::ifstream ifile(filename, std::ifstream::binary);
				debug_assert(ifile);

				read_level(ifile, level);
			}

			entities.reserve(0 + 2 + level.statics.size() + level.platforms.size() + level.trigger_multiples.size());

			const json content = json::parse(std::ifstream("res/desc.json"));

			// need a local map to connect name with entity
			std::unordered_map<std::string, ::engine::Entity> objects;

			// player start
			// trigger timer
			{
				entities.push_back(engine::Entity::create());
				{
					const auto & box = level.trigger_timer.start;
					engine::graphics::data::CuboidC data = {
						box.matrix,
						box.dimensions[0],
						box.dimensions[1],
						box.dimensions[2],
						0x8844cccc
					};
					engine::graphics::renderer::add(entities.back(), data);
				}
				{
					const auto & box = level.trigger_timer.stop;
					engine::graphics::data::CuboidC data = {
						box.matrix,
						box.dimensions[0],
						box.dimensions[1],
						box.dimensions[2],
						0x8844cccc
					};
					engine::graphics::renderer::add(entities.back(), data);
				}
			}
			{
				// statics
				for (unsigned int i = 0; i < level.statics.size(); i++)
				{
					const auto & box = level.statics[i].box;

					core::maths::Vector3f::array_type dimensions;
					box.scalation.get_aligned(dimensions);

					entities.push_back(engine::Entity::create());
					{
						std::vector<engine::physics::ShapeData> shapes;
						shapes.push_back(engine::physics::ShapeData {
								engine::physics::ShapeData::Type::BOX,
								engine::physics::Material::WOOD,
								1.f,
								core::maths::Vector3f{0.f, 0.f, 0.f},
								core::maths::Quaternionf{1.f, 0.f, 0.f, 0.f},
								engine::physics::ShapeData::Geometry{engine::physics::ShapeData::Geometry::Box{dimensions[0], dimensions[1], dimensions[2]} }});

						engine::physics::ActorData data {engine::physics::ActorData::Type::STATIC, engine::physics::ActorData::Behaviour::DEFAULT, box.translation, box.rotation, shapes};

						::engine::physics::post_create(entities.back(), data);
					}
					{
						engine::graphics::data::CuboidC data = {
							make_translation_matrix(box.translation) * make_matrix(box.rotation),
							dimensions[0] * 2.f,
							dimensions[1] * 2.f,
							dimensions[2] * 2.f,
							0xffcc4400
						};
						engine::graphics::renderer::add(entities.back(), data);
					}
				}
				// dynamic
				for (unsigned int i = 0; i<level.dynamics.size(); i++)
				{
					const auto & box = level.dynamics[i].box;

					entities.push_back(engine::Entity::create());
					{
						const auto translation = box.matrix.get_column<3>();
						core::maths::Vector4f::array_type buffer;
						translation.get_aligned(buffer);

						debug_printline(0xffffffff, box.dimensions[0], ", ", box.dimensions[1], ", ", box.dimensions[2]);

						std::vector<engine::physics::ShapeData> shapes;
						shapes.push_back(engine::physics::ShapeData {
							engine::physics::ShapeData::Type::BOX,
							engine::physics::Material::WOOD,
							0.01f,
							core::maths::Vector3f {0.f, 0.f, 0.f},
							core::maths::Quaternionf {1.f, 0.f, 0.f, 0.f},
							engine::physics::ShapeData::Geometry {
								engine::physics::ShapeData::Geometry::Box {
									box.dimensions[0]*0.5f,
									box.dimensions[1]*0.5f,
									box.dimensions[2]*0.5f}}});

						engine::physics::ActorData data {engine::physics::ActorData::Type::DYNAMIC, engine::physics::ActorData::Behaviour::DEFAULT, box.translation, box.rotation, shapes};

						::engine::physics::post_create(entities.back(), data);
					}
					{
						engine::graphics::data::CuboidC data = {
							box.matrix,
							box.dimensions[0],
							box.dimensions[1],
							box.dimensions[2],
							0xffcc44bb
						};
						engine::graphics::renderer::add(entities.back(), data);
					}
				}
			}
			// placeholders
			{
				const json & list = content["placeholders"];

				for (unsigned int i = 0; i < level.placeholders.size(); i++)
				{
					const auto & placeholder = level.placeholders[i];
					const std::string type = list[placeholder.name]["type"];

					load(placeholder, type);
				}
			}
			// platforms
			{
				const json list = content["platforms"];

				for (unsigned int i = 0; i < level.platforms.size(); i++)
				{
					auto & platform = level.platforms[i];

					// check for description
					const auto val = list.find(platform.name);

					if (val==list.end()) continue;

					const json data = *val;

					const auto entity = engine::Entity::create();

					objects.emplace(platform.name, entity);
					entities.push_back(entity);

					if (!platform.animation.actions.empty())
					{
						const auto asset = engine::extras::Asset(platform.animation.name);

						engine::animation::add(asset, std::move(platform.animation));
						engine::animation::add_model(entity, asset);
					}

					const auto & box = platform.box;
					core::maths::Vector3f::array_type dimensions;
					box.scalation.get_aligned(dimensions);
					{
						engine::graphics::data::CuboidC data = {
							make_translation_matrix(box.translation) * make_matrix(box.rotation),
							dimensions[0] * 2.f,
							dimensions[1] * 2.f,
							dimensions[2] * 2.f,
							0xffcc4400
						};
						engine::graphics::renderer::add(entities.back(), data);
					}
					{
						std::vector<engine::physics::ShapeData> shapes;
						shapes.push_back(engine::physics::ShapeData {
							engine::physics::ShapeData::Type::BOX,
							engine::physics::Material::WOOD,
							1.f,
							core::maths::Vector3f {0.f, 0.f, 0.f},
							core::maths::Quaternionf {1.f, 0.f, 0.f, 0.f},
							engine::physics::ShapeData::Geometry {engine::physics::ShapeData::Geometry::Box {dimensions[0], dimensions[1], dimensions[2]}}});

						engine::physics::ActorData data {engine::physics::ActorData::Type::KINEMATIC, engine::physics::ActorData::Behaviour::OBSTACLE, box.translation, box.rotation, shapes};

						::engine::physics::post_create(entity, data);
					}
				}
			}
			// trigger multiples
			{
				const json list = content["triggers"];

				for (unsigned int i = 0; i<level.trigger_multiples.size(); i++)
				{
					auto & trigger = level.trigger_multiples[i];

					// check for description
					const auto val = list.find(trigger.name);

					if (val==list.end()) continue;

					const json data = *val;

					const auto & box = trigger.box;

					entities.push_back(engine::Entity::create());
					{
						engine::graphics::data::CuboidC data = {
							box.matrix,
							box.dimensions[0],
							box.dimensions[1],
							box.dimensions[2],
							0x4444cccc
						};
						engine::graphics::renderer::add(entities.back(), data);
					}
					{
						const auto translation = box.matrix.get_column<3>();
						core::maths::Vector4f::array_type buffer;
						translation.get_aligned(buffer);

						debug_printline(0xffffffff, box.dimensions[0], ", ", box.dimensions[1], ", ", box.dimensions[2]);

						std::vector<engine::physics::ShapeData> shapes;
						shapes.push_back(engine::physics::ShapeData {
							engine::physics::ShapeData::Type::BOX,
							engine::physics::Material::WOOD,
							1.f,
							core::maths::Vector3f {0.f, 0.f, 0.f},
							core::maths::Quaternionf {1.f, 0.f, 0.f, 0.f},
							engine::physics::ShapeData::Geometry {engine::physics::ShapeData::Geometry::Box {box.dimensions[0]*0.5f, box.dimensions[1]*0.5f, box.dimensions[2]*0.5f}}});

						engine::physics::ActorData data {engine::physics::ActorData::Type::TRIGGER, engine::physics::ActorData::Behaviour::TRIGGER, box.translation, box.rotation, shapes};

						::engine::physics::post_create(entities.back(), data);
					}

					// parse targets of the trigger
					std::vector<::engine::Entity> targets;

					for (const std::string & targetName : data["targets"])
					{
						const auto t = objects.find(targetName);

						if (t!=objects.end()) targets.push_back(t->second);
						else debug_printline(0xffffffff, "Trigger does not have a platform target: ", targetName);
					}

					// parse target type
					const std::string type_str = data["type"];
					::gameplay::characters::trigger_t::Type type;

					if (type_str=="door") type = ::gameplay::characters::trigger_t::Type::DOOR;
					else continue;

					// add trigger to "characters"
					::gameplay::characters::post_add_trigger(::gameplay::characters::trigger_t {entities.back(), type, targets});
				}
			}
			// activators
			{
				const json list = content["activators"];

				for (const json & activator : list)
				{
					const std::string name = activator["name"];
					const std::string animatiton = activator["animation"];

					const auto & target = objects.find(name);
					if (target!=objects.end())
					{
						engine::animation::update(target->second, engine::animation::action {animatiton, true});
					}
				}
			}
		}
		void destroy()
		{
			// TODO: free entities
		}
	}
}
