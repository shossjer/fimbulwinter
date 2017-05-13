
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
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>
#include <utility/string.hpp>

#include <fstream>
#include <vector>
#include <unordered_map>

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

			for (const auto & mesh : level.statics)
			{
				engine::graphics::renderer::asset_definition_t assetDef;

				assetDef.meshs.resize(1);

				const uint32_t r = mesh.color[0] * 255;
				const uint32_t g = mesh.color[1] * 255;
				const uint32_t b = mesh.color[2] * 255;

				assetDef.meshs[0].color = r + (g << 8) + (b << 16) + (0xff << 24);
				assetDef.meshs[0].vertices = mesh.vertices;
				assetDef.meshs[0].normals = mesh.normals;
				assetDef.meshs[0].triangles = mesh.triangles;

				const engine::Asset asset = mesh.name;
				engine::graphics::renderer::add(asset, assetDef);

				engine::graphics::renderer::asset_instance_t assetInst;

				engine::transform_t trans{ Vector3f{0.f, 0.f, 0.f}, Quaternionf{ 1.f, 0.f, 0.f, 0.f } };

				assetInst.asset = asset;
				assetInst.modelview = trans.matrix();

				engine::graphics::renderer::add(engine::Entity::create(), assetInst);
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
