
#ifndef CORE_PLACEHOLDERSTRUCTURER_HPP
#define CORE_PLACEHOLDERSTRUCTURER_HPP

#include "core/container/Buffer.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"
#include "core/ReadStream.hpp"
#include "core/serialization.hpp"

#include <algorithm>
#include <vector>
#include <string>

namespace core
{
	class PlaceholderStructurer
	{
	private:
		ReadStream stream;

	public:
		PlaceholderStructurer(ReadStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			std::string name;
			read_string(name);
			debug_printline("mesh name: ", name);

			static_assert(member_table<T>::has("matrix"), "");
			member_table<T>::call("matrix", x, [&](auto & y){ read_matrix(y); });
			static_assert(member_table<T>::has("vertices"), "");
			member_table<T>::call("vertices", x, [&](auto & y){ read_array<float>(y, 3); });
			static_assert(member_table<T>::has("uvs"), "");
			member_table<T>::call("uvs", x, [&](auto & y){ read_array<float>(y, 2); });
			static_assert(member_table<T>::has("normals"), "");
			member_table<T>::call("normals", x, [&](auto & y){ read_array<float>(y, 3); });
			static_assert(member_table<T>::has("weights"), "");
			member_table<T>::call("weights", x, [&](auto & y){ read_weights(y); });
			static_assert(member_table<T>::has("triangles"), "");
			member_table<T>::call("triangles", x, [&](auto & y){ read_array<unsigned int>(y, 3); });
		}
	private:
		template <typename T>
		int read_array(container::Buffer & x, int element_size = 1)
		{
			uint16_t count;
			read_count(count);
			debug_printline(count, " array count in placeholder '", stream.filepath(), "'");

			x.resize<T>(count * element_size);
			read_bytes(x.data(), count * element_size * sizeof(T));
			return count * element_size;
		}
		template <typename T, typename P>
		int read_array(P &, int /*element_size*/ = 1)
		{
			debug_fail("attempting to read array into a non array type in placeholder '", stream.filepath(), "'");
			return 0;
		}

		void read_bytes(char * ptr, ext::usize size)
		{
			if (!debug_verify(!stream.done()))
				return; // todo error

			const ext::usize amount_read = stream.read_all(ptr, size);
			if (amount_read < size)
			{
				debug_printline("warning: eof while reading '", stream.filepath(), "'"); // todo error
			}
		}

		void read_count(uint16_t & count)
		{
			read_bytes(reinterpret_cast<char *>(&count), sizeof(uint16_t));
		}
		template <typename T>
		void read_count(T &)
		{
			debug_fail("attempting to read count into a non count type in placeholder '", stream.filepath(), "'");
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
			debug_fail("attempting to read matrix into a non matrix type in placeholder '", stream.filepath(), "'");
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
			debug_fail("attempting to read string into a non string type in placeholder '", stream.filepath(), "'");
		}

		void read_value(float & value)
		{
			read_bytes(reinterpret_cast<char *>(&value), sizeof(float));
		}
		template <typename T>
		void read_value(T &)
		{
			debug_fail("attempting to read value into a non value type in placeholder '", stream.filepath(), "'");
		}

		template <typename T>
		int read_weights(std::vector<T> & x)
		{
			uint16_t nweights;
			read_count(nweights);
			debug_printline(nweights, " weight count in placeholder '", stream.filepath(), "'");

			x.resize(nweights);
			for (auto & w : x)
			{
				uint16_t ngroups;
				read_count(ngroups);
				debug_assert(ngroups > 0);

				// read the first weight value
				static_assert(member_table<T>::has("index"), "");
				member_table<T>::call("index", w, [&](auto & y){ read_count(y); });
				static_assert(member_table<T>::has("value"), "");
				member_table<T>::call("value", w, [&](auto & y){ read_value(y); });

				// skip remaining ones if any for now
				for (auto i = 1; i < ngroups; i++)
				{
					uint16_t unused_index;
					read_count(unused_index);

					float unused_value;
					read_value(unused_value);
				}
			}
			return nweights;
		}
		template <typename T>
		int read_weights(T &)
		{
			debug_fail("attempting to read weights into a non array type in placeholder '", stream.filepath(), "'");
			return 0;
		}
	};
}

#endif /* CORE_PLACEHOLDERSTRUCTURER_HPP */
