
#include "armature.hpp"

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>
#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include <cstdint>
#include <fstream>
#include <vector>

namespace
{
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
	void read_quaternion(std::ifstream & stream, core::maths::Quaternionf & quat)
	{
		core::maths::Quaternionf::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		quat.set_aligned(buffer);
	}
	void read_vector(std::ifstream & stream, core::maths::Vector3f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set_aligned(buffer);
	}
	void read_vector(std::ifstream & stream, core::maths::Vector4f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set(buffer[0], buffer[1], buffer[2], 1.f);
	}
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (&buffer)[N])
	{
		uint16_t len; // including null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
		debug_assert(len <= N);

		stream.read(buffer, len);
	}

	struct Joint
	{
		char name[16]; // arbitrary

		core::maths::Matrix4x4f rel_matrix;
		core::maths::Matrix4x4f inv_matrix;

		uint16_t parenti;
		uint16_t nchildren;
		// uint16_t childreni[6]; // arbitrary
	};

	void read_joint_chain(std::ifstream & stream, std::vector<Joint> & joints, const int parenti)
	{
		const int mei = joints.size();
		joints.emplace_back();
		Joint & me = joints.back();

		read_string(stream, me.name);
		// debug_printline(0xffffffff, me.name);

		read_matrix(stream, me.rel_matrix);
		read_matrix(stream, me.inv_matrix);

		me.parenti = parenti;

		read_count(stream, me.nchildren);
		// debug_printline(0xffffffff, me.nchildren, " children");
		for (int i = 0; i < static_cast<int>(me.nchildren); i++)
			read_joint_chain(stream, joints, mei);
	}

	struct Action
	{
		struct Frame
		{
			struct Channel
			{
				core::maths::Vector3f translation;
				core::maths::Quaternionf rotation;
				core::maths::Vector3f scale;
			};

			std::vector<Channel> channels;

			Frame(const int nchannels) :
				channels(nchannels)
			{}
		};

		char name[24]; // arbitrary

		int32_t length;

		std::vector<Frame> frames;
	};

	void read_action(std::ifstream & stream, std::vector<Action> & actions, const int njoints)
	{
		// const int mei = actions.size();
		actions.emplace_back();
		Action & me = actions.back();

		read_string(stream, me.name);
		debug_printline(0xffffffff, me.name);

		stream.read(reinterpret_cast<char *>(& me.length), sizeof(int32_t));
		debug_printline(0xffffffff, me.length);

		me.frames.reserve(me.length + 1);
		for (int framei = 0; framei <= me.length; framei++)
		{
			me.frames.emplace_back(njoints);
			for (int i = 0; i < njoints; i++)
			{
				uint16_t index;
				read_count(stream, index);

				read_vector(stream, me.frames.back().channels[index].translation);
				read_quaternion(stream, me.frames.back().channels[index].rotation);
				read_vector(stream, me.frames.back().channels[index].scale);
			}
		}
	}

	struct Armature
	{
		char name[64]; // arbitrary

		core::maths::Matrix4x4f matrix;

		uint16_t njoints;
		// uint16_t nroots;
		uint16_t nactions;

		std::vector<Joint> joints;
		std::vector<Action> actions;

		Armature()
			:
			matrix{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f }
		{}
	};

	void read_armature(std::ifstream & stream, Armature & armature)
	{
		read_string(stream, armature.name);
		debug_printline(0xffffffff, "armature name: ", armature.name);

		read_matrix(stream, armature.matrix);

		read_count(stream, armature.njoints);
		debug_printline(0xffffffff, "armature njoints: ", armature.njoints);

		uint16_t nroots;
		read_count(stream, /*armature.*/nroots);
		debug_printline(0xffffffff, "armature nroots: ", /*armature.*/nroots);

		armature.joints.reserve(armature.njoints);
		// for (int i = 0; i < static_cast<int>(armature.nroots); i++)
		while (armature.joints.size() < armature.njoints)
			read_joint_chain(stream, armature.joints, -1);

		read_count(stream, armature.nactions);
		debug_printline(0xffffffff, "armature nactions: ", armature.nactions);

		armature.actions.reserve(armature.nactions);
		while (armature.actions.size() < armature.nactions)
			read_action(stream, armature.actions, armature.njoints);
	}

	struct Weight
	{
		uint16_t index;
		float value;
	};

	void read_weight(std::ifstream & stream, Weight & weight)
	{
		read_count(stream, weight.index);

		stream.read(reinterpret_cast<char *>(& weight.value), sizeof(float));
	}

	struct Mesh
	{
		char name[64]; // arbitrary

		core::maths::Matrix4x4f matrix;

		uint16_t nvertices;
		uint16_t nedges;

		std::vector<core::maths::Vector4f> vertices;
		std::vector<std::pair<uint16_t, uint16_t>> edges;
		std::vector<Weight> weights;

		Mesh()
			:
			matrix{ 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f }
		{}
	};

	void read_mesh(std::ifstream & stream, Mesh & mesh)
	{
		read_string(stream, mesh.name);
		debug_printline(0xffffffff, "mesh name: ", mesh.name);

		read_matrix(stream, mesh.matrix);

		read_count(stream, mesh.nvertices);
		debug_printline(0xffffffff, "mesh nvertices: ", mesh.nvertices);

		mesh.vertices.resize(mesh.nvertices);
		for (auto && vertex : mesh.vertices)
			read_vector(stream, vertex);

		read_count(stream, mesh.nedges);
		debug_printline(0xffffffff, "mesh nedges: ", mesh.nedges);

		mesh.edges.resize(mesh.nedges);
		for (auto && edge : mesh.edges)
		{
			read_count(stream, edge.first);
			read_count(stream, edge.second);
		}

		mesh.weights.resize(mesh.nvertices);
		for (int i = 0; i < static_cast<int>(mesh.nvertices); i++)
		{
			uint16_t ngroups;
			read_count(stream, ngroups);
			debug_assert(ngroups == 1);

			read_weight(stream, mesh.weights[i]);
			// debug_printline(0xffffffff, "vertex ", i, ": ", mesh.weights[i].index, ", ", mesh.weights[i].value);
		}
	}

	struct Character
	{
		const Armature & armature;
		const Mesh & mesh;

		std::vector<core::maths::Matrix4x4f> matrices;
		std::vector<core::maths::Matrix4x4f> matrix_pallet;

		std::vector<core::maths::Vector4f> vertices;

		Character(const Armature & armature,
		          const Mesh & mesh) :
			armature(armature),
			mesh(mesh),
			matrices(armature.njoints),
			matrix_pallet(armature.njoints),
			vertices(mesh.nvertices)
		{}

		void draw()
		{
			for (int i = 0; i < static_cast<int>(mesh.nvertices); i++)
				vertices[i] = matrix_pallet[mesh.weights[i].index] * mesh.vertices[i];

			glLineWidth(2.f);
			glBegin(GL_LINES);
			for (auto && edge : mesh.edges)
			{
				core::maths::Vector4f::array_type buffer1;
				vertices[edge.first].get_aligned(buffer1);
				glVertex4fv(buffer1);
				core::maths::Vector4f::array_type buffer2;
				vertices[edge.second].get_aligned(buffer2);
				glVertex4fv(buffer2);
			}
			glEnd();
		}

		void update()
		{
			static int framei = 0;
			framei += 1;
			if (framei >= armature.actions[0].length) framei = 0;

			int rooti = 0;
			while (rooti < static_cast<int>(armature.njoints))
				// rooti = update(rooti, core::maths::Matrix4x4f::rotation(core::maths::degreef{-90.f}, 1.f, 0.f, 0.f), framei);
				rooti = update(rooti, core::maths::Matrix4x4f::identity(), framei);

			for (int i = 0; i < static_cast<int>(armature.njoints); i++)
				matrix_pallet[i] = matrices[i] * armature.joints[i].inv_matrix;
		}
		int update(const int mei, const core::maths::Matrix4x4f & parent_matrix, const int framei)
		{
			const auto pos = armature.actions[0].frames[framei].channels[mei].translation;
			const auto rot = armature.actions[0].frames[framei].channels[mei].rotation;
			const auto scale = armature.actions[0].frames[framei].channels[mei].scale;

			const auto pose =
				make_translation_matrix(pos) *
				make_matrix(rot) *
				make_scale_matrix(scale);

			matrices[mei] = parent_matrix * pose;

			int childi = mei + 1;
			for (int i = 0; i < static_cast<int>(armature.joints[mei].nchildren); i++)
				childi = update(childi, matrices[mei], framei);
			return childi;
		}
	};

	Armature armature;
	Mesh mesh;
	std::vector<Character> characters;
}

namespace engine
{
	namespace model
	{
		void init()
		{
			{
				std::ifstream file("res/my_armature", std::ifstream::binary | std::ifstream::in);

				read_armature(file, armature);
			}
			{
				std::ifstream file("res/my_mesh", std::ifstream::binary | std::ifstream::in);

				read_mesh(file, mesh);
			}
			characters.emplace_back(armature, mesh);
		}

		void draw()
		{
			glTranslatef(0.f, 0.f, 0.f);
			//glRotatef(90.f, 0.f, 1.f, 0.f);
			static double deg = 0.;
			if ((deg += .5) >= 360.) deg -= 360.;
			glRotated(deg, 0., 1., 0.);
			glScalef(3.f, 3.f, 3.f);

			glRotatef(-90.f, 1.f, 0.f, 0.f);

			for (auto && character : characters)
			{
				character.draw();
			}
		}

		void update()
		{
			for (auto && character : characters)
			{
				character.update();
			}
		}
	}
}
