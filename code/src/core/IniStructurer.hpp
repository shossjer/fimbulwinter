
#ifndef CORE_INISTRUCTURER_HPP
#define CORE_INISTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/string.hpp"

#include <string>
#include <vector>

namespace core
{
	class IniStructurer
	{
	private:
		std::vector<char> buffer;

		std::string filename;

	public:
		IniStructurer(int size, std::string filename)
			: buffer(size)
			, filename(std::move(filename))
		{}
		IniStructurer(std::vector<char> && buffer, std::string filename)
			: buffer(std::move(buffer))
			, filename(std::move(filename))
		{}

	public:
		char * data() { return buffer.data(); }

		template <typename T>
		void read(T & x)
		{
			int p = 0;

			read_key_values(p, x);
			read_headers(p, x);
		}
	private:
		void fast_forward(int & p)
		{
			debug_assert(p < buffer.size());
			while (buffer[p] == ';' ||
			       (buffer[p] == '\n' || buffer[p] == '\r'))
			{
				skip_line(p);
				if (p >= buffer.size())
					return;
			}
		}

		void skip_line(int & p)
		{
			debug_assert(p < buffer.size());
			while (!(buffer[p] == '\n' || buffer[p] == '\r'))
			{
				p++;
				if (p >= buffer.size())
					return;
			}
			while (buffer[p] == '\n' || buffer[p] == '\r')
			{
				p++;
				if (p >= buffer.size())
					return;
			}
		}

		bool is_header(const int & p) const
		{
			debug_assert(p < buffer.size());
			return buffer[p] == '[';
		}

		template <typename T>
		void read_header(int & p, T & x)
		{
			debug_assert(is_header(p));
			p++; // '['
			if (p >= buffer.size())
				return;

			const int header_from = p;
			while (buffer[p] != ']')
			{
				p++;
				if (p >= buffer.size())
					return;
			}
			const int header_to = p;
			const utility::string_view header_name(buffer.data() + header_from, header_to - header_from);

			p++; // ']'
			if (p >= buffer.size())
				return;

			skip_line(p);

			core::member_table<T>::call(header_name, x, [&](auto & y){ return read_key_values(p, y); });
		}

		template <typename T>
		void read_headers(int & p, T & x)
		{
			while (p < buffer.size())
			{
				fast_forward(p);
				if (p >= buffer.size())
					return;

				debug_assert(is_header(p));
				read_header(p, x);
			}
		}

		void parse_value(std::string & x, utility::string_view value_string)
		{
			x = std::string(value_string.begin(), value_string.end());
		}
		template <typename T,
		          REQUIRES((!std::is_enum<T>::value)),
		          REQUIRES((!std::is_class<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			utility::from_string(std::string(value_string.begin(), value_string.end()), x);
		}
		template <typename T,
		          REQUIRES((std::is_enum<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			if (core::value_table<T>::has(value_string))
			{
				x = core::value_table<T>::get(value_string);
			}
			else
			{
				debug_fail("what is this enum thing!?");
			}
		}
		template <typename T,
		          REQUIRES((std::is_class<T>::value))>
		void parse_value(T & x, utility::string_view value_string)
		{
			debug_fail("this is a strange type");
		}

		template <typename T>
		void read_key_value(int & p, T & x)
		{
			const int key_from = p;
			while (buffer[p] != '=')
			{
				p++;
				if (p >= buffer.size())
					return;
			}
			const int key_to = p;
			const utility::string_view key_name(buffer.data() + key_from, key_to - key_from);

			p++; // '='
			if (p >= buffer.size())
				return;

			const int value_from = p;
			while (!(buffer[p] == '\n' || buffer[p] == '\r'))
			{
				p++;
				if (p >= buffer.size())
					return;
			}
			const int value_to = p;
			const utility::string_view value_string(buffer.data() + value_from, value_to - value_from);

			core::member_table<T>::call(key_name, x, [&](auto & y){ return parse_value(y, value_string); });
		}

		template <typename T>
		void read_key_values(int & p, T & x)
		{
			while (p < buffer.size())
			{
				fast_forward(p);
				if (p >= buffer.size())
					return;

				if (is_header(p))
					return;

				read_key_value(p, x);
			}
		}
	};
}

#endif /* CORE_INISTRUCTURER_HPP */
