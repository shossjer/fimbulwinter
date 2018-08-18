
#ifndef CORE_LEVELSTRUCTURER_HPP
#define CORE_LEVELSTRUCTURER_HPP

#include "core/container/Buffer.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/serialization.hpp"

#include "utility/concepts.hpp"

#include <algorithm>
#include <vector>
#include <string>

namespace core
{
	class LevelStructurer
	{
	private:
		std::vector<char> bytes;

		std::string filename;

	public:
		LevelStructurer(int size, std::string filename)
			: bytes(size)
			, filename(std::move(filename))
		{}
		LevelStructurer(std::vector<char> && bytes, std::string filename)
			: bytes(std::move(bytes))
			, filename(std::move(filename))
		{}

	public:
		char * data() { return bytes.data(); }

		template <typename T>
		void read(T & x)
		{
			int curr = 0;
			static_assert(serialization<T>::has("meshes"), "");
			serialization<T>::call("meshes", x, [&](auto & y){ read_meshes(curr, y); });
			static_assert(serialization<T>::has("placeholders"), "");
			serialization<T>::call("placeholders", x, [&](auto & y){ read_placeholders(curr, y); });
		}
	private:
		template <typename T>
		int read_array(int & curr, container::Buffer & x, int element_size = 1)
		{
			uint16_t count;
			read_count(curr, count);

			x.resize<T>(count * element_size);
			read_bytes(curr, x.data(), count * element_size * sizeof(T));
			return count * element_size;
		}
		template <typename T, typename P>
		int read_array(int & curr, P & x, int element_size = 1)
		{
			debug_fail("attempting to read array into a non array type in level '", filename, "'");
			return 0;
		}

		void read_bytes(int & curr, char * ptr, int size)
		{
			const auto from = bytes.begin() + curr;
			const auto to = from + size;
			debug_assert(to <= bytes.end());
			std::copy(from, to, ptr);
			curr += size;
		}

		void read_color(int & curr, float (& buffer)[3])
		{
			read_bytes(curr, reinterpret_cast<char *>(buffer), sizeof(buffer));
		}
		template <typename T>
		void read_color(int & curr, T & x)
		{
			debug_fail("attempting to read color into a non color type in level '", filename, "'");
		}

		void read_count(int & curr, uint16_t & count)
		{
			read_bytes(curr, reinterpret_cast<char *>(&count), sizeof(uint16_t));
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
			debug_fail("attempting to read matrix into a non matrix type in level '", filename, "'");
		}

		template <typename T,
		          REQUIRES((serialization<T>::has("name") &&
		                    serialization<T>::has("matrix") &&
		                    serialization<T>::has("color") &&
		                    serialization<T>::has("vertices") &&
			                 serialization<T>::has("normals") &&
			                 serialization<T>::has("triangles")))>
		int read_meshes(int & curr, std::vector<T> & x)
		{
			uint16_t nmeshes;
			read_count(curr, nmeshes);

			x.resize(nmeshes);
			for (auto & m : x)
			{
				static_assert(serialization<T>::has("name"), "");
				serialization<T>::call("name", m, [&](auto & y){ read_string(curr, y); });
				static_assert(serialization<T>::has("matrix"), "");
				serialization<T>::call("matrix", m, [&](auto & y){ read_matrix(curr, y); });

				static_assert(serialization<T>::has("color"), "");
				serialization<T>::call("color", m, [&](auto & y){ read_color(curr, y); });

				static_assert(serialization<T>::has("vertices"), "");
				const auto nvertices = serialization<T>::call("vertices", m, [&](auto & y){ return read_array<float>(curr, y, 3); });
				static_assert(serialization<T>::has("normals"), "");
				const auto nnormals = serialization<T>::call("normals", m, [&](auto & y){ return read_array<float>(curr, y, 3); });
				debug_assert(nvertices == nnormals);
				static_assert(serialization<T>::has("triangles"), "");
				serialization<T>::call("triangles", m, [&](auto & y){ return read_array<uint16_t>(curr, y, 3); });
			}
			return nmeshes;
		}
		template <typename T>
		int read_meshes(int & curr, T & x)
		{
			debug_fail("attempting to read meshes into a non array type in level '", filename, "'");
			return 0;
		}

		template <typename T,
		          REQUIRES((serialization<T>::has("name") &&
		                    serialization<T>::has("translation") &&
			                 serialization<T>::has("rotation") &&
		                    serialization<T>::has("scale")))>
		int read_placeholders(int & curr, std::vector<T> & x)
		{
			uint16_t nplaceholders;
			read_count(curr, nplaceholders);

			x.resize(nplaceholders);
			for (auto & p : x)
			{
				static_assert(serialization<T>::has("name"), "");
				serialization<T>::call("name", p, [&](auto & y){ read_string(curr, y); });
				static_assert(serialization<T>::has("translation"), "");
				serialization<T>::call("translation", p, [&](auto & y){ read_vector(curr, y); });
				static_assert(serialization<T>::has("rotation"), "");
				serialization<T>::call("rotation", p, [&](auto & y){ read_quaternion(curr, y); });
				static_assert(serialization<T>::has("scale"), "");
				serialization<T>::call("scale", p, [&](auto & y){ read_vector(curr, y); });
			}
			return nplaceholders;
		}
		template <typename T>
		int read_placeholders(int & curr, T & x)
		{
			debug_fail("attempting to read placeholders into a non array type in level '", filename, "'");
			return 0;
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

#endif /* CORE_LEVELSTRUCTURER_HPP */
