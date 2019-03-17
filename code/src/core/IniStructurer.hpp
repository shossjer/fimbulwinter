
#ifndef CORE_INISTRUCTURER_HPP
#define CORE_INISTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/string.hpp"

#include <exception>
#include <string>
#include <vector>

namespace core
{
	class SmallFile
	{
	private:
		std::vector<char> buffer;
		int p = 0;

		std::string filename;

	public:
		SmallFile(int size, std::string filename)
			: buffer(size)
			, filename(std::move(filename))
		{}

	public:
		char * data() { return buffer.data(); }

		const char * data(int pos) const
		{
			debug_assert(pos >= 0);

			return buffer.data() + pos;
		}

		char peek() const
		{
			debug_assert(p < buffer.size());

			return buffer[p];
		}

		int pos() const
		{
			return p;
		}

		bool valid() const
		{
			return p < buffer.size();
		}

		void next()
		{
			p++;
		}
	};

	class IniStructurer
	{
	private:
		SmallFile & stream;

	public:
		IniStructurer(SmallFile & stream)
			: stream(stream)
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			read_key_values(x);
			read_headers(x);
		}
	private:
		bool fast_forward()
		{
			while (stream.peek() == ';' || is_newline())
			{
				skip_line();
				if (!stream.valid())
					return false;
			}
			return true;
		}

		void skip_line()
		{
			while (!is_newline())
			{
				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			while (is_newline())
			{
				stream.next();
				if (!stream.valid())
					return;
			}
		}

		bool is_header() const
		{
			return stream.peek() == '[';
		}

		bool is_newline() const
		{
			return stream.peek() == '\n' || stream.peek() == '\r';
		}

		template <typename T>
		void read_header(T & x)
		{
			debug_assert(is_header());

			stream.next(); // '['
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			const int header_from = stream.pos();
			while (stream.peek() != ']')
			{
				if (is_newline())
					throw std::runtime_error("unexpected eol");

				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int header_to = stream.pos();
			const utility::string_view header_name(stream.data(header_from), header_to - header_from);

			stream.next(); // ']'
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			core::member_table<T>::call(header_name, x, [&](auto & y){ return read_key_values(y); });
		}

		template <typename T>
		void read_headers(T & x)
		{
			while (stream.valid())
			{
				fast_forward();
				if (!stream.valid())
					return;

				debug_assert(is_header());
				read_header(x);
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
		void read_key_value(T & x)
		{
			const int key_from = stream.pos();
			while (stream.peek() != '=')
			{
				if (is_newline())
					throw std::runtime_error("unexpected eol");

				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int key_to = stream.pos();
			const utility::string_view key_name(stream.data(key_from), key_to - key_from);

			stream.next(); // '='
			if (!stream.valid())
				throw std::runtime_error("unexpected eof");

			const int value_from = stream.pos();
			while (!is_newline())
			{
				stream.next();
				if (!stream.valid())
					throw std::runtime_error("unexpected eof");
			}
			const int value_to = stream.pos();
			const utility::string_view value_string(stream.data(value_from), value_to - value_from);

			core::member_table<T>::call(key_name, x, [&](auto & y){ return parse_value(y, value_string); });
		}

		template <typename T>
		void read_key_values(T & x)
		{
			while (stream.valid())
			{
				fast_forward();
				if (!stream.valid())
					return;
				if (is_header())
					return;

				read_key_value(x);
			}
		}
	};
}

#endif /* CORE_INISTRUCTURER_HPP */
