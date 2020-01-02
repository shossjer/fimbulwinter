
#ifndef CORE_LEVELSTRUCTURER_HPP
#define CORE_LEVELSTRUCTURER_HPP

#include "core/container/Buffer.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/ReadStream.hpp"
#include "core/serialization.hpp"

#include "utility/concepts.hpp"

#include <algorithm>
#include <vector>
#include <string>

namespace core
{
#if defined(_MSC_VER) && _MSC_VER <= 1916
	template <typename T>
	struct LevelStructurerHelper
	{
		static const bool IsMesh =
			member_table<T>::has("name") &&
			member_table<T>::has("matrix") &&
			member_table<T>::has("color") &&
			member_table<T>::has("vertices") &&
			member_table<T>::has("normals") &&
			member_table<T>::has("triangles");

		static const bool IsPlaceholder =
			member_table<T>::has("name") &&
			member_table<T>::has("translation") &&
			member_table<T>::has("rotation") &&
			member_table<T>::has("scale");
	};
#endif

	class LevelStructurer
	{
	private:
		core::ReadStream stream;

	public:
		LevelStructurer(core::ReadStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			static_assert(member_table<T>::has("meshes"), "");
			member_table<T>::call("meshes", x, [&](auto & y){ read_meshes(y); });
			static_assert(member_table<T>::has("placeholders"), "");
			member_table<T>::call("placeholders", x, [&](auto & y){ read_placeholders(y); });
		}
	private:
		template <typename T>
		int read_array(container::Buffer & x, int element_size = 1)
		{
			uint16_t count;
			read_count(count);

			x.resize<T>(count * element_size);
			read_bytes(x.data(), count * element_size * sizeof(T));
			return count * element_size;
		}
		template <typename T, typename P>
		int read_array(P &, int /*element_size*/ = 1)
		{
			debug_fail("attempting to read array into a non array type in level '", stream.filename, "'");
			return 0;
		}

		void read_bytes(char * ptr, int64_t size)
		{
			if (!stream.valid())
				throw std::runtime_error("unexpected eol");
			const int64_t amount_read = stream.read_block(ptr, size);
			if (amount_read < size)
				throw std::runtime_error("unexpected eol");
		}

		void read_color(float (& buffer)[3])
		{
			read_bytes(reinterpret_cast<char *>(buffer), sizeof(buffer));
		}
		template <typename T>
		void read_color(T &)
		{
			debug_fail("attempting to read color into a non color type in level '", stream.filename, "'");
		}

		void read_count(uint16_t & count)
		{
			read_bytes(reinterpret_cast<char *>(&count), sizeof(uint16_t));
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
			debug_fail("attempting to read matrix into a non matrix type in level '", stream.filename, "'");
		}

		template <typename T,
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((LevelStructurerHelper<T>::IsMesh))>
#else
		          REQUIRES((member_table<T>::has("name") &&
		                    member_table<T>::has("matrix") &&
		                    member_table<T>::has("color") &&
		                    member_table<T>::has("vertices") &&
		                    member_table<T>::has("normals") &&
		                    member_table<T>::has("triangles")))>
#endif
		int read_meshes(std::vector<T> & x)
		{
			uint16_t nmeshes;
			read_count(nmeshes);

			x.resize(nmeshes);
			for (auto & m : x)
			{
				static_assert(member_table<T>::has("name"), "");
				member_table<T>::call("name", m, [&](auto & y){ read_string(y); });
				static_assert(member_table<T>::has("matrix"), "");
				member_table<T>::call("matrix", m, [&](auto & y){ read_matrix(y); });

				static_assert(member_table<T>::has("color"), "");
				member_table<T>::call("color", m, [&](auto & y){ read_color(y); });

				static_assert(member_table<T>::has("vertices"), "");
				const auto nvertices = member_table<T>::call("vertices", m, [&](auto & y){ return read_array<float>(y, 3); });
				static_assert(member_table<T>::has("normals"), "");
				const auto nnormals = member_table<T>::call("normals", m, [&](auto & y){ return read_array<float>(y, 3); });
				debug_assert(nvertices == nnormals);
				static_assert(member_table<T>::has("triangles"), "");
				member_table<T>::call("triangles", m, [&](auto & y){ return read_array<uint16_t>(y, 3); });
			}
			return nmeshes;
		}
		template <typename T>
		int read_meshes(T &)
		{
			debug_fail("attempting to read meshes into a non array type in level '", stream.filename, "'");
			return 0;
		}

		template <typename T,
#if defined(_MSC_VER) && _MSC_VER <= 1916
		          REQUIRES((LevelStructurerHelper<T>::IsPlaceholder))>
#else
		          REQUIRES((member_table<T>::has("name") &&
		                    member_table<T>::has("translation") &&
		                    member_table<T>::has("rotation") &&
		                    member_table<T>::has("scale")))>
#endif
		int read_placeholders(std::vector<T> & x)
		{
			uint16_t nplaceholders;
			read_count(nplaceholders);

			x.resize(nplaceholders);
			for (auto & p : x)
			{
				static_assert(member_table<T>::has("name"), "");
				member_table<T>::call("name", p, [&](auto & y){ read_string(y); });
				static_assert(member_table<T>::has("translation"), "");
				member_table<T>::call("translation", p, [&](auto & y){ read_vector(y); });
				static_assert(member_table<T>::has("rotation"), "");
				member_table<T>::call("rotation", p, [&](auto & y){ read_quaternion(y); });
				static_assert(member_table<T>::has("scale"), "");
				member_table<T>::call("scale", p, [&](auto & y){ read_vector(y); });
			}
			return nplaceholders;
		}
		template <typename T>
		int read_placeholders(T &)
		{
			debug_fail("attempting to read placeholders into a non array type in level '", stream.filename, "'");
			return 0;
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
			debug_fail("attempting to read quaternion into a non quaternion type in level '", stream.filename, "'");
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
			debug_fail("attempting to read string into a non string type in level '", stream.filename, "'");
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
			debug_fail("attempting to read vector into a non vector type in level '", stream.filename, "'");
		}
	};
}

#endif /* CORE_LEVELSTRUCTURER_HPP */
