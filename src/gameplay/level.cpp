
#include "level.hpp"

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

	void read_level(std::ifstream & ifile, level_t & level)
	{
		read_player_start(ifile, level.player_start);
		read_trigger_timer(ifile, level.trigger_timer);

		read_statics(ifile, level.statics);
		read_dynamics(ifile, level.dynamics);
		read_platforms(ifile, level.platforms);

		read_trigger_multiples(ifile, level.trigger_multiples);

		read_meshes(ifile, level.meshes);
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

			// TODO: make finished
		}
		void destroy()
		{
			// TODO: free entities
		}
	}
}
