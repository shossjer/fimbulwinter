
#include "level.hpp"

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>
#include <engine/graphics/renderer.hpp>

#include <fstream>
#include <vector>

namespace
{
	struct theline_t
	{
		core::maths::Matrix4x4f matrix;
		core::container::Buffer vertices;
		core::container::Buffer edges;

		theline_t()
		{
		}
	};
	struct player_t
	{
		core::maths::Matrix4x4f matrix;

		player_t()
		{
		}
	};
	struct mesh_t
	{
		core::maths::Matrix4x4f matrix;
		core::container::Buffer vertices;
		core::container::Buffer triangles;
		core::container::Buffer normals;
	};

	struct level_t
	{
		theline_t theline;
		player_t player;
		std::vector<mesh_t> meshes;
	};

	void read_count(std::ifstream & stream, uint16_t & count)
	{
		stream.read(reinterpret_cast<char *>(& count), sizeof(uint16_t));
	}
	void read_matrix(std::ifstream & stream, core::maths::Matrix4x4f & matrix)
	{
		core::maths::Matrix4x4f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		matrix.set_aligned(buffer);
	}
	void read_vector(std::ifstream & stream, core::maths::Vector3f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set_aligned(buffer);
	}

	void read_vertices(std::ifstream & ifile, core::container::Buffer & vertices)
	{
		uint16_t nvertices;
		read_count(ifile, nvertices);

		vertices.resize<float>(3 * std::size_t{nvertices});
		ifile.read(vertices.data(), vertices.size());
	}
	void read_edges(std::ifstream & ifile, core::container::Buffer & edges)
	{
		uint16_t nedges;
		read_count(ifile, nedges);

		edges.resize<uint16_t>(2 * std::size_t{nedges});
		ifile.read(edges.data(), edges.size());
	}
	void read_triangles(std::ifstream & ifile, core::container::Buffer & triangles, core::container::Buffer & normals)
	{
		uint16_t ntriangles;
		read_count(ifile, ntriangles);

		triangles.resize<uint16_t>(3 * std::size_t{ntriangles});
		ifile.read(triangles.data(), triangles.size());
		normals.resize<float>(3 * std::size_t{ntriangles});
		ifile.read(normals.data(), normals.size());
	}

	void read_mesh(std::ifstream & ifile, core::container::Buffer & vertices, core::container::Buffer & normals, core::container::Buffer & triangles)
	{
		uint16_t nvertices;
		read_count(ifile, nvertices);

		vertices.resize<float>(3 * std::size_t{nvertices});
		ifile.read(vertices.data(), vertices.size());
		normals.resize<float>(3 * std::size_t{nvertices});
		ifile.read(normals.data(), normals.size());

		uint16_t ntriangles;
		read_count(ifile, ntriangles);

		triangles.resize<uint16_t>(3 * std::size_t{ntriangles});
		ifile.read(triangles.data(), triangles.size());
	}

	void read_theline(std::ifstream & ifile, theline_t & theline)
	{
		read_matrix(ifile, theline.matrix);
		read_vertices(ifile, theline.vertices);
		read_edges(ifile, theline.edges);
	}
	void read_player(std::ifstream & ifile, player_t & player)
	{
		read_matrix(ifile, player.matrix);
	}
	void read_meshes(std::ifstream & ifile, std::vector<mesh_t> & meshes)
	{
		uint16_t nmeshes;
		read_count(ifile, nmeshes);

		meshes.resize(nmeshes);
		for (auto && mesh : meshes)
		{
			read_matrix(ifile, mesh.matrix);
			read_mesh(ifile, mesh.vertices, mesh.normals, mesh.triangles);
		}
	}

	void read_level(std::ifstream & ifile, level_t & level)
	{
		read_theline(ifile, level.theline);
		read_player(ifile, level.player);
		read_meshes(ifile, level.meshes);
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

			entities.reserve(1 + level.meshes.size());
			// theline
			entities.push_back(engine::Entity::create());
			{
				engine::graphics::data::LineC data;
				data.vertices = std::move(level.theline.vertices);
				data.edges = std::move(level.theline.edges);
				engine::graphics::renderer::add(entities.back(), data);
			}
			// meshes
			for (std::size_t n = 0; n < level.meshes.size(); n++)
			{
				entities.push_back(engine::Entity::create());
				{
					engine::graphics::data::MeshC data;
					data.vertices = std::move(level.meshes[n].vertices);
					data.triangles = std::move(level.meshes[n].triangles);
					data.normals = std::move(level.meshes[n].normals);
					engine::graphics::renderer::add(entities.back(), data);
				}
			}
		}
		void destroy()
		{
			// TODO: free entities
		}
	}
}
