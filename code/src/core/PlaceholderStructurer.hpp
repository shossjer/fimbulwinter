
#ifndef CORE_PLACEHOLDERSTRUCTURER_HPP
#define CORE_PLACEHOLDERSTRUCTURER_HPP

#include "core/container/Buffer.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/serialization.hpp"

#include <algorithm>
#include <vector>
#include <string>

namespace core
{
	class PlaceholderStructurer
	{
	private:
		std::vector<char> bytes;

		std::string filename;

	public:
		PlaceholderStructurer(int size, std::string filename)
			: bytes(size)
			, filename(std::move(filename))
		{}
		PlaceholderStructurer(std::vector<char> && bytes, std::string filename)
			: bytes(std::move(bytes))
			, filename(std::move(filename))
		{}

	public:
		char * data() { return bytes.data(); }

		template <typename T>
		void read(T & x)
		{
			int curr = 0;

			std::string name;
			read_string(curr, name);
			debug_printline("mesh name: ", name);

			static_assert(member_table<T>::has("matrix"), "");
			member_table<T>::call("matrix", x, [&](auto & y){ read_matrix(curr, y); });
			static_assert(member_table<T>::has("vertices"), "");
			member_table<T>::call("vertices", x, [&](auto & y){ read_array<float>(curr, y, 3); });
			static_assert(member_table<T>::has("uvs"), "");
			member_table<T>::call("uvs", x, [&](auto & y){ read_array<float>(curr, y, 2); });
			static_assert(member_table<T>::has("normals"), "");
			member_table<T>::call("normals", x, [&](auto & y){ read_array<float>(curr, y, 3); });
			static_assert(member_table<T>::has("weights"), "");
			member_table<T>::call("weights", x, [&](auto & y){ read_weights(curr, y); });
			static_assert(member_table<T>::has("triangles"), "");
			member_table<T>::call("triangles", x, [&](auto & y){ read_array<unsigned int>(curr, y, 3); });
		}
	private:
		template <typename T>
		int read_array(int & curr, container::Buffer & x, int element_size = 1)
		{
			uint16_t count;
			read_count(curr, count);
			debug_printline(count, " array count in placeholder '", filename, "'");

			x.resize<T>(count * element_size);
			read_bytes(curr, x.data(), count * element_size * sizeof(T));
			return count * element_size;
		}
		template <typename T, typename P>
		int read_array(int & curr, P & x, int element_size = 1)
		{
			debug_fail("attempting to read array into a non array type in placeholder '", filename, "'");
			return 0;
		}

		void read_bytes(int & curr, char * ptr, int size)
		{
			if (bytes.size() - curr < size)
			{
				debug_printline("warning: eof while reading '", filename, "'");
				size = bytes.size() - curr;
			}
			const auto from = bytes.begin() + curr;
			const auto to = from + size;
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
			debug_fail("attempting to read count into a non count type in placeholder '", filename, "'");
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
			debug_fail("attempting to read matrix into a non matrix type in placeholder '", filename, "'");
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
			debug_fail("attempting to read string into a non string type in placeholder '", filename, "'");
		}

		void read_value(int & curr, float & value)
		{
			read_bytes(curr, reinterpret_cast<char *>(&value), sizeof(float));
		}
		template <typename T>
		void read_value(int & curr, T & x)
		{
			debug_fail("attempting to read value into a non value type in placeholder '", filename, "'");
		}

		template <typename T>
		int read_weights(int & curr, std::vector<T> & x)
		{
			uint16_t nweights;
			read_count(curr, nweights);
			debug_printline(nweights, " weight count in placeholder '", filename, "'");

			x.resize(nweights);
			for (auto & w : x)
			{
				uint16_t ngroups;
				read_count(curr, ngroups);
				debug_assert(ngroups > 0);

				// read the first weight value
				static_assert(member_table<T>::has("index"), "");
				member_table<T>::call("index", w, [&](auto & y){ read_count(curr, y); });
				static_assert(member_table<T>::has("value"), "");
				member_table<T>::call("value", w, [&](auto & y){ read_value(curr, y); });

				// skip remaining ones if any for now
				for (auto i = 1; i < ngroups; i++)
				{
					uint16_t unused_index;
					read_count(curr, unused_index);

					float unused_value;
					read_value(curr, unused_value);
				}
			}
			return nweights;
		}
		template <typename T>
		int read_weights(int & curr, T & x)
		{
			debug_fail("attempting to read weights into a non array type in placeholder '", filename, "'");
			return 0;
		}
	};
}

#endif /* CORE_PLACEHOLDERSTRUCTURER_HPP */
