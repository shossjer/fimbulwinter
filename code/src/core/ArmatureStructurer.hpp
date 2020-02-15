
#ifndef CORE_ARMATURESTRUCTURER_HPP
#define CORE_ARMATURESTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/ReadStream.hpp"
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
		ReadStream read_stream_;

	public:
		ArmatureStructurer(ReadStream && read_stream)
			: read_stream_(std::move(read_stream))
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", x, [&](auto & y){ read_string(y); });

			uint16_t njoints;
			read_count(njoints);
			debug_printline("armature njoints: ", njoints);

			uint16_t nroots;
			read_count(nroots);
			debug_printline("armature nroots: ", nroots);

			static_assert(member_table<T>::has("joints"), "");
			member_table<T>::call("joints", x, [&](auto & y){ read_joints(y, njoints); });

			uint16_t nactions;
			read_count(nactions);
			debug_printline("armature nactions: ", nactions);

			static_assert(member_table<T>::has("actions"), "");
			member_table<T>::call("actions", x, [&](auto & y){ read_actions(y, njoints, nactions); });
		}
	private:
		template <typename T>
		void read_action(T & x, int njoints)
		{
			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", x, [&](auto & y){ read_string(y); });

			int32_t length;
			read_length(length);
			debug_printline(length);

			core::assign<member_table<T>::find("length")>(x, [length](){ return length; });

			static_assert(member_table<T>::has("frames"), "");
			member_table<T>::call("frames", x, [&](auto & y){ read_frames(y, njoints, length); });

			uint8_t has_positions;
			read_byte(has_positions);
			debug_printline(has_positions ? "has positions" : "has NOT positions");

			if (has_positions)
			{
				static_assert(member_table<T>::has("positions"), "");
				member_table<T>::call("positions", x, [&](auto & y){ read_positions(y, length); });
			}

			uint8_t has_orientations;
			read_byte(has_orientations);
			debug_printline(has_orientations ? "has orientations" : "has NOT orientations");

			if (has_orientations)
			{
				static_assert(member_table<T>::has("orientations"), "");
				member_table<T>::call("orientations", x, [&](auto & y){ read_orientations(y, length); });
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
		void read_actions(std::vector<T> & actions, int njoints, int nactions)
		{
			actions.reserve(nactions);
			while (actions.size() < std::size_t(nactions))
			{
				actions.emplace_back();
				read_action(actions.back(), njoints);
			}
		}
		template <typename T>
		void read_actions(T &, int /*njoints*/, int /*nactions*/)
		{
			debug_fail("attempting to read actions into a non array type in armature '", read_stream_.filepath(), "'");
		}

		void read_byte(uint8_t & byte)
		{
			read_bytes(reinterpret_cast<char *>(&byte), sizeof(uint8_t));
		}

		void read_bytes(char * ptr, ext::usize size)
		{
			if (!debug_verify(!read_stream_.done()))
				return; // todo error

			debug_verify(read_stream_.read_all(ptr, size) == size, "unexpected eol"); // todo error
		}

		void read_count(uint16_t & count)
		{
			read_bytes(reinterpret_cast<char *>(&count), sizeof(uint16_t));
		}
		template <typename T>
		void read_count(T &)
		{
			debug_fail("attempting to read count into a non count type in armature '", read_stream_.filepath(), "'");
		}

		template <typename T>
		void read_frame(std::vector<T> & channels, int index)
		{
			static_assert(member_table<T>::has("translation"), "");
			member_table<T>::call("translation", channels[index], [&](auto & y){ read_vector(y); });
			static_assert(member_table<T>::has("rotation"), "");
			member_table<T>::call("rotation", channels[index], [&](auto & y){ read_quaternion(y); });
			static_assert(member_table<T>::has("scale"), "");
			member_table<T>::call("scale", channels[index], [&](auto & y){ read_vector(y); });
		}

		template <typename T,
		          REQUIRES((has_lookup_table<T>::value)),
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((ArmatureStructurerHelper<T>::IsFrame))>
#else
		          REQUIRES((member_table<T>::has("channels")))>
#endif
		void read_frames(std::vector<T> & frames, int njoints, int length)
		{
			frames.reserve(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				frames.emplace_back(njoints);
				for (int i = 0; i < njoints; i++)
				{
					uint16_t index;
					read_count(index);

					static_assert(member_table<T>::has("channels"), "");
					member_table<T>::call("channels", frames.back(), [&](auto & y){ read_frame(y, index); });
				}
			}
		}
		template <typename T>
		void read_frames(T &, int /*njoints*/, int /*length*/)
		{
			debug_fail("attempting to read frames into a non array type in armature '", read_stream_.filepath(), "'");
		}

		template <typename T>
		void read_joint_chain(std::vector<T> & joints, int parenti)
		{
			const int mei = debug_cast<int>(joints.size());
			joints.emplace_back();
			T & me = joints.back();

			static_assert(member_table<T>::has("name"), "");
			member_table<T>::call("name", me, [&](auto & y){ read_string(y); });

			if (member_table<T>::has("matrix"))
			{
				member_table<T>::call("matrix", me, [&](auto & y){ read_matrix(y); });
			}
			else
			{
				core::maths::Matrix4x4f unused;
				read_matrix(unused);
			}
			if (member_table<T>::has("inv_matrix"))
			{
				member_table<T>::call("inv_matrix", me, [&](auto & y){ read_matrix(y); });
			}
			else
			{
				core::maths::Matrix4x4f unused;
				read_matrix(unused);
			}

			core::assign<member_table<T>::find("parent")>(me, [parenti]() { debug_assert(parenti < 0x10000); return uint16_t(parenti); });

			uint16_t nchildren;
			read_count(nchildren);
			core::assign<member_table<T>::find("children")>(me, [nchildren]() { return nchildren; });
			for (int i = 0; i < static_cast<int>(nchildren); i++)
			{
				read_joint_chain(joints, mei);
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
		void read_joints(std::vector<T> & x, int count)
		{
			x.reserve(count);
			while (x.size() < std::size_t(count))
			{
				read_joint_chain(x, -1);
			}
		}
		template <typename T>
		void read_joints(T &, int /*count*/)
		{
			debug_fail("attempting to read joints into a non array type in armature '", read_stream_.filepath(), "'");
		}

		void read_length(int32_t & length)
		{
			read_bytes(reinterpret_cast<char *>(&length), sizeof(int32_t));
		}

		void read_matrix(core::maths::Matrix4x4f & x)
		{
			core::maths::Matrix4x4f::array_type buffer;
			read_bytes(reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_matrix(T &)
		{
			debug_fail("attempting to read matrix into a non matrix type in armature '", read_stream_.filepath(), "'");
		}

		template <typename T,
		          REQUIRES((mpl::is_same<T, core::maths::Quaternionf>::value))>
		void read_orientations(std::vector<T> & orientations, int length)
		{
			orientations.resize(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				read_quaternion(orientations[framei]);
			}
		}
		template <typename T>
		void read_orientations(T &, int /*length*/)
		{
			debug_fail("attempting to read orientations into a non array type in armature '", read_stream_.filepath(), "'");
		}

		template <typename T,
		          REQUIRES((mpl::is_same<T, core::maths::Vector3f>::value))>
		void read_positions(std::vector<T> & positions, int length)
		{
			positions.resize(length + 1);
			for (int framei = 0; framei <= length; framei++)
			{
				read_vector(positions[framei]);
			}
		}
		template <typename T>
		void read_positions(T &, int /*length*/)
		{
			debug_fail("attempting to read positions into a non array type in armature '", read_stream_.filepath(), "'");
		}

		void read_quaternion(core::maths::Quaternionf & x)
		{
			core::maths::Quaternionf::array_type buffer;
			read_bytes(reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_quaternion(T &)
		{
			debug_fail("attempting to read quaternion into a non quaternion type in level '", read_stream_.filepath(), "'");
		}

		template <std::size_t N>
		void read_string(char (& buffer)[N])
		{
			uint16_t len; // excluding null character
			read_count(len);
			debug_assert(len < N);

			read_bytes(buffer, len);
			buffer[len] = '\0';
		}
		void read_string(std::string & x)
		{
			char buffer[64]; // arbitrary
			read_string(buffer);
			x = buffer;
		}
		template <typename T>
		void read_string(T &)
		{
			debug_fail("attempting to read string into a non string type in level '", read_stream_.filepath(), "'");
		}

		void read_vector(core::maths::Vector3f & x)
		{
			core::maths::Vector3f::array_type buffer;
			read_bytes(reinterpret_cast<char *>(buffer), sizeof(buffer));
			x.set_aligned(buffer);
		}
		template <typename T>
		void read_vector(T &)
		{
			debug_fail("attempting to read vector into a non vector type in level '", read_stream_.filepath(), "'");
		}
	};
}

#endif /* CORE_ARMATURESTRUCTURER_HPP */
