
#ifndef CORE_ARMATURESTRUCTURER_HPP
#define CORE_ARMATURESTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/serialization.hpp"

#include <cstddef>
#include <vector>
#include <string>

namespace core
{
#if defined(_MSC_VER) && _MSC_VER <= 1916
	template <typename T>
	struct ArmatureStructurerHelper
	{
		static const bool IsAction =
			member_table<T>::has("name") &&
			member_table<T>::has("length") &&
			member_table<T>::has("frames") &&
			member_table<T>::has("positions") &&
			member_table<T>::has("orientations");

		static const bool IsFrame = member_table<T>::has("channels");

		static const bool IsJoint =
			member_table<T>::has("name") &&
			member_table<T>::has("inv_matrix") &&
			member_table<T>::has("parent") &&
			member_table<T>::has("children");
	};
#endif

	class ArmatureStructurer
	{
	private:
		std::vector<char> bytes;

		std::string filename;

	public:
		ArmatureStructurer(int size, std::string filename)
			: bytes(size)
			, filename(std::move(filename))
		{}
		ArmatureStructurer(std::vector<char> && bytes, std::string filename)
			: bytes(std::move(bytes))
			, filename(std::move(filename))
		{}

	public:
		char * data() { return bytes.data(); }

		template <typename T>
		void read(T & x)
		{
			int curr = 0;
			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", x, [&](auto & y){ read_string(curr, y); });

			uint16_t njoints;
			read_count(curr, njoints);
			debug_printline("armature njoints: ", njoints);

			uint16_t nroots;
			read_count(curr, nroots);
			debug_printline("armature nroots: ", nroots);

			static_assert(member_table<T>::has("joints"), "");
			member_table<T>::call("joints", x, [&](auto & y){ read_joints(curr, y, njoints); });

			uint16_t nactions;
			read_count(curr, nactions);
			debug_printline("armature nactions: ", nactions);

			static_assert(member_table<T>::has("actions"), "");
			member_table<T>::call("actions", x, [&](auto & y){ read_actions(curr, y, njoints, nactions); });
		}
	private:
		template <typename T>
		void read_action(int & curr, T & x, int njoints)
		{
			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", x, [&](auto & y){ read_string(curr, y); });

			int32_t length;
			read_length(curr, length);
			debug_printline(length);

			static_assert(member_table<T>::has("length"), "");
			member_table<T>::call("length", x, TryAssign<int32_t>(length));

			static_assert(member_table<T>::has("frames"), "");
			member_table<T>::call("frames", x, [&](auto & y){ read_frames(curr, y, njoints, length); });

			uint8_t has_positions;
			read_byte(curr, has_positions);
			debug_printline(has_positions ? "has positions" : "has NOT positions");

			if (has_positions)
			{
				static_assert(member_table<T>::has("positions"), "");
				member_table<T>::call("positions", x, [&](auto & y){ read_positions(curr, y, length); });
			}

			uint8_t has_orientations;
			read_byte(curr, has_orientations);
			debug_printline(has_orientations ? "has orientations" : "has NOT orientations");

			if (has_orientations)
			{
				static_assert(member_table<T>::has("orientations"), "");
				member_table<T>::call("orientations", x, [&](auto & y){ read_orientations(curr, y, length); });
			}
		}

		template <typename T,
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((ArmatureStructurerHelper<T>::IsAction))>
#else
		          REQUIRES((member_table<T>::has("name") &&
		                    member_table<T>::has("length") &&
		                    member_table<T>::has("frames") &&
		                    member_table<T>::has("positions") &&
		                    member_table<T>::has("orientations")))>
#endif
		void read_actions(int & curr, std::vector<T> & actions, int njoints, int nactions)
		{
			actions.reserve(nactions);
			while (actions.size() < nactions)
			{
				actions.emplace_back();
				read_action(curr, actions.back(), njoints);
			}
		}
		template <typename T>
		void read_actions(int & curr, T & x, int njoints, int nactions)
		{
			debug_fail("attempting to read actions into a non array type in armature '", filename, "'");
		}

		void read_byte(int & curr, uint8_t & byte)
		{
			read_bytes(curr, reinterpret_cast<char *>(&byte), sizeof(uint8_t));
		}

		void read_bytes(int & curr, char * ptr, int size)
		{
			const auto from = bytes.begin() + curr;
			const auto to = from + size;
			debug_assert(to <= bytes.end());
			std::copy(from, to, ptr);
			curr += size;
		}

		void read_count(int & curr, uint16_t & count)
		{
			read_bytes(curr, reinterpret_cast<char *>(&count), sizeof(uint16_t));
		}
		template <typename T>
		void read_count(int & curr, T & x)
		{
			debug_fail("attempting to read count into a non count type in armature '", filename, "'");
		}

		template <typename T>
		void read_frame(int & curr, std::vector<T> & channels, int index)
		{
			static_assert(member_table<T>::has("translation"), "");
			member_table<T>::call("translation", channels[index], [&](auto & y){ read_vector(curr, y); });
			static_assert(member_table<T>::has("rotation"), "");
			member_table<T>::call("rotation", channels[index], [&](auto & y){ read_quaternion(curr, y); });
			static_assert(member_table<T>::has("scale"), "");
			member_table<T>::call("scale", channels[index], [&](auto & y){ read_vector(curr, y); });
		}

		template <typename T,
		          REQUIRES((has_lookup_table<T>::value)),
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((ArmatureStructurerHelper<T>::IsFrame))>
#else
		          REQUIRES((member_table<T>::has("channels")))>
#endif
		void read_frames(int & curr, std::vector<T> & frames, int njoints, int length)
		{
			frames.reserve(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				frames.emplace_back(njoints);
				for (int i = 0; i < njoints; i++)
				{
					uint16_t index;
					read_count(curr, index);

					static_assert(member_table<T>::has("channels"), "");
					member_table<T>::call("channels", frames.back(), [&](auto & y){ read_frame(curr, y, index); });
				}
			}
		}
		template <typename T>
		void read_frames(int & curr, T & x, int njoints, int length)
		{
			debug_fail("attempting to read frames into a non array type in armature '", filename, "'");
		}

		template <typename T>
		void read_joint_chain(int & curr, std::vector<T> & joints, int parenti)
		{
			const int mei = joints.size();
			joints.emplace_back();
			T & me = joints.back();

			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", me, [&](auto & y){ read_string(curr, me.name); });

			if (member_table<T>::has("matrix"))
			{
				member_table<T>::call("matrix", me, [&](auto & y){ read_matrix(curr, y); });
			}
			else
			{
				core::maths::Matrix4x4f unused;
				read_matrix(curr, unused);
			}
			if (member_table<T>::has("inv_matrix"))
			{
				member_table<T>::call("inv_matrix", me, [&](auto & y){ read_matrix(curr, y); });
			}
			else
			{
				core::maths::Matrix4x4f unused;
				read_matrix(curr, unused);
			}

			static_assert(member_table<T>::has("parent"), "");
			member_table<T>::call("parent", me, TryAssign<int>(parenti));

			uint16_t nchildren;
			read_count(curr, nchildren);
			static_assert(member_table<T>::has("children"), "");
			member_table<T>::call("children", me, TryAssign<uint16_t>(nchildren));
			for (int i = 0; i < static_cast<int>(nchildren); i++)
			{
				read_joint_chain(curr, joints, mei);
			}
		}

		template <typename T,
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((ArmatureStructurerHelper<T>::IsJoint))>
#else
		          REQUIRES((member_table<T>::has("name") &&
		                    member_table<T>::has("inv_matrix") &&
		                    member_table<T>::has("parent") &&
		                    member_table<T>::has("children")))>
#endif
		void read_joints(int & curr, std::vector<T> & x, int count)
		{
			x.reserve(count);
			while (x.size() < count)
			{
				read_joint_chain(curr, x, -1);
			}
		}
		template <typename T>
		void read_joints(int & curr, T & x, int count)
		{
			debug_fail("attempting to read joints into a non array type in armature '", filename, "'");
		}

		void read_length(int & curr, int32_t & length)
		{
			read_bytes(curr, reinterpret_cast<char *>(&length), sizeof(int32_t));
		}

		void read_matrix(int & curr, core::maths::Matrix4x4f & x)
		{
			core::maths::Matrix4x4f::array_type buffer;
			read_bytes(curr, reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_matrix(int & curr, T & x)
		{
			debug_fail("attempting to read matrix into a non matrix type in armature '", filename, "'");
		}

		template <typename T,
		          REQUIRES((mpl::is_same<T, core::maths::Quaternionf>::value))>
		void read_orientations(int & curr, std::vector<T> & orientations, int length)
		{
			orientations.resize(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				read_quaternion(curr, orientations[framei]);
			}
		}
		template <typename T>
		void read_orientations(int & curr, T & x, int length)
		{
			debug_fail("attempting to read orientations into a non array type in armature '", filename, "'");
		}

		template <typename T,
		          REQUIRES((mpl::is_same<T, core::maths::Vector3f>::value))>
		void read_positions(int & curr, std::vector<T> & positions, int length)
		{
			positions.resize(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				read_vector(curr, positions[framei]);
			}
		}
		template <typename T>
		void read_positions(int & curr, T & x, int length)
		{
			debug_fail("attempting to read positions into a non array type in armature '", filename, "'");
		}

		void read_quaternion(int & curr, core::maths::Quaternionf & x)
		{
			core::maths::Quaternionf::array_type buffer;
			read_bytes(curr, reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_quaternion(int & curr, T & x)
		{
			debug_fail("attempting to read quaternion into a non quaternion type in level '", filename, "'");
		}

		template <std::size_t N>
		void read_string(int & curr, char (& buffer)[N])
		{
			uint16_t len; // excluding null character
			read_count(curr, len);
			debug_assert(len < N);

			read_bytes(curr, buffer, len);
			buffer[len] = '\0';
		}
		void read_string(int & curr, std::string & x)
		{
			char buffer[64]; // arbitrary
			read_string(curr, buffer);
			x = buffer;
		}
		template <typename T>
		void read_string(int & curr, T & x)
		{
			debug_fail("attempting to read string into a non string type in level '", filename, "'");
		}

		void read_vector(int & curr, core::maths::Vector3f & x)
		{
			core::maths::Vector3f::array_type buffer;
			read_bytes(curr, reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_vector(int & curr, T & x)
		{
			debug_fail("attempting to read vector into a non vector type in level '", filename, "'");
		}
	};
}

#endif /* CORE_ARMATURESTRUCTURER_HPP */
