
#include "level.hpp"

#include "level_placeholder.hpp"

#include <gameplay/gamestate.hpp>
#include <gameplay/ui.hpp>

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>
#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/model/loader.hpp>
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>
#include <utility/string.hpp>

#include <fstream>
#include <vector>

namespace
{
	using namespace gameplay::level;

	struct player_start_t
	{
		core::maths::Matrix4x4f matrix;
	};

	struct mesh_t
	{
		std::string name;
		core::maths::Matrix4x4f matrix;
		float color[3];
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;

		mesh_t() {}
	};

	struct level_t
	{
		std::vector<mesh_t> statics;

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

	void read_meshes(std::ifstream & ifile, std::vector<mesh_t> & meshes)
	{
		uint16_t nmeshes;
		read_count(ifile, nmeshes);

		meshes.resize(nmeshes);
		for (auto & mesh : meshes)
		{
			read_string(ifile, mesh.name);
			read_matrix(ifile, mesh.matrix);

			// color
			read_vector(ifile, mesh.color);

			std::vector<float> vertexes;
			uint16_t nvertices;
			read_count(ifile, nvertices);

			mesh.vertices.resize<float>(nvertices * 3);
			ifile.read(reinterpret_cast<char *>(mesh.vertices.data()), mesh.vertices.size());

			uint16_t nnormals;
			read_count(ifile, nnormals);
			debug_assert(nvertices == nnormals);
			mesh.normals.resize<float>(nnormals * 3);
			ifile.read(reinterpret_cast<char *>(mesh.normals.data()), mesh.normals.size());

			uint16_t ntriangles;
			read_count(ifile, ntriangles);
			mesh.triangles.resize<uint16_t>(ntriangles * 3);
			ifile.read(reinterpret_cast<char *>(mesh.triangles.data()), mesh.triangles.size());
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
			read_vector(ifile, placeholder.transform.pos);
			read_quaternion(ifile, placeholder.transform.quat);
			read_vector(ifile, placeholder.scale);
		}
	}

	void read_level(std::ifstream & ifile, level_t & level)
	{
		read_meshes(ifile, level.statics);

		read_placeholders(ifile, level.placeholders);
	}
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

			engine::physics::camera::set(engine::physics::camera::Bounds{
				Vector3f{-6.f, 0.f, -4.f},
				Vector3f{6.f, 10.f, 12.f} });

			for (const auto & data : level.statics)
			{
				const engine::Asset asset = data.name;

				engine::graphics::renderer::post_register_mesh(
					asset,
					engine::graphics::data::Mesh{
						data.vertices,
						data.triangles,
						data.normals,
						core::container::Buffer{} });

				std::vector<engine::graphics::data::CompC::asset> assets;

				const uint32_t r = data.color[0] * 255;
				const uint32_t g = data.color[1] * 255;
				const uint32_t b = data.color[2] * 255;
				assets.emplace_back(
					engine::graphics::data::CompC::asset{
						asset,
						r + (g << 8) + (b << 16) + (0xff << 24) });

				const auto entity = engine::Entity::create();
				engine::graphics::renderer::post_add_component(
					entity,
					engine::graphics::data::CompC{
						data.matrix,
						Vector3f{1.f, 1.f, 1.f},
						assets});
				engine::graphics::renderer::post_make_obstruction(entity);
			}

			for (const auto & ph : level.placeholders)
			{
				load(ph);
			}
		}
		void destroy()
		{
			// TODO: free entities
		}
	}
}
