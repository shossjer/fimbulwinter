
#include "Armature.hpp"

#include <core/debug.hpp>

namespace
{
	using Action = engine::animation::Armature::Action;
	using Armature = engine::animation::Armature;
	using Joint = engine::animation::Armature::Joint;

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
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (&buffer)[N])
	{
		uint16_t len; // including null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
		debug_assert(len <= N);

		stream.read(buffer, len);
	}

	void read_joint_chain(std::ifstream & stream, std::vector<Joint> & joints, const int parenti)
	{
		const int mei = joints.size();
		joints.emplace_back();
		Joint & me = joints.back();

		read_string(stream, me.name);

		// read_matrix(stream, me.matrix);
		core::maths::Matrix4x4f unused;
		read_matrix(stream, unused);
		read_matrix(stream, me.inv_matrix);

		me.parenti = parenti;

		read_count(stream, me.nchildren);
		for (int i = 0; i < static_cast<int>(me.nchildren); i++)
			read_joint_chain(stream, joints, mei);
	}

	void read_action(std::ifstream & stream, std::vector<Action> & actions, const int njoints)
	{
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

		uint8_t has_positions;
		stream.read(reinterpret_cast<char *>(& has_positions), sizeof(uint8_t));
		debug_printline(0xffffffff, has_positions ? "has positions" : "has NOT positions");

		if (has_positions)
		{
			me.positions.resize(me.length + 1);
			for (int framei = 0; framei <= me.length; framei++)
			{
				read_vector(stream, me.positions[framei]);
			}
		}
	}

	void read_armature(std::ifstream & stream, Armature & armature)
	{
		read_string(stream, armature.name);
		debug_printline(0xffffffff, "armature name: ", armature.name);

		read_count(stream, armature.njoints);
		debug_printline(0xffffffff, "armature njoints: ", armature.njoints);

		uint16_t nroots;
		read_count(stream, nroots);
		debug_printline(0xffffffff, "armature nroots: ", nroots);

		armature.joints.reserve(armature.njoints);
		while (armature.joints.size() < armature.njoints)
			read_joint_chain(stream, armature.joints, -1);

		read_count(stream, armature.nactions);
		debug_printline(0xffffffff, "armature nactions: ", armature.nactions);

		armature.actions.reserve(armature.nactions);
		while (armature.actions.size() < armature.nactions)
			read_action(stream, armature.actions, armature.njoints);
	}
}

namespace engine
{
	namespace animation
	{
		void Armature::read(std::ifstream & stream)
		{
			read_armature(stream, *this);
		}
	}
}
