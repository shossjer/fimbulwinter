
#ifndef CORE_SHADERSTRUCTURER_HPP
#define CORE_SHADERSTRUCTURER_HPP

#include "core/BufferedStream.hpp"
#include "core/debug.hpp"
#include "core/serialization.hpp"

#include "utility/string.hpp"

#include <algorithm>
#include <vector>
#include <string>

namespace core
{
	struct beginline_t { explicit constexpr beginline_t(int) {} };
	struct endline_t { explicit constexpr endline_t(int) {} };
	struct endoffile_t { explicit constexpr endoffile_t(int) {} };
	struct newline_t { explicit constexpr newline_t(int) {} };
	struct whitespace_t { explicit constexpr whitespace_t(int) {} };

	constexpr beginline_t beginline{ 0 };
	constexpr endline_t endline{ 0 };
	constexpr endoffile_t endoffile{ 0 };
	constexpr newline_t newline{ 0 };
	constexpr whitespace_t whitespace{ 0 };

	class ShaderStructurer
	{
	private:
		BufferedStream stream;

	public:
		ShaderStructurer(ReadStream && stream)
			: stream(std::move(stream))
		{}

	public:
		template <typename T>
		void read(T & x)
		{
			debug_verify(parse("[vertex]", newline), "expected vertex section in '", stream.filepath(), "'");
			static_assert(core::member_table<T>::has("inputs"), "");
			// if (core::member_table<T>::has("inputs"))
			{
				while (parse("[bind", whitespace))
				{
					if (core::member_table<T>::call("inputs", x, [&](auto & y){ return read_name_value(y); }))
					{
						debug_verify(parse("]", newline), "expected ending bracket in '", stream.filepath(), "'");
					}
					else
					{
						skip_line();
					}
				}
			}
			// else
			// {
			// 	while (parse("["))
			// 	{
			// 		skip_line();
			// 	}
			// }
			stream.consume();
			const std::ptrdiff_t vertex_source_begin = stream.pos();
			const std::ptrdiff_t vertex_source_end = find(beginline, "[fragment]", endline);
			if (core::member_table<T>::has("vertex_source"))
			{
				parse_region(vertex_source_begin, vertex_source_end, x.vertex_source);
			}
			else
			{
				skip_region(vertex_source_begin, vertex_source_end);
			}
			// debug_printline(std::string(stream.data(vertex_source_begin), stream.data(vertex_source_end)));

			stream.consume();
			debug_verify(parse("[fragment]", newline), "expected fragment section in '", stream.filepath(), "'");
			static_assert(core::member_table<T>::has("outputs"), "");
			// if (core::member_table<T>::has("outputs"))
			{
				while (parse("[bind", whitespace))
				{
					if (core::member_table<T>::call("outputs", x, [&](auto & y){ return read_name_value(y); }))
					{
						debug_verify(parse("]", newline), "expected ending bracket in '", stream.filepath(), "'");
					}
					else
					{
						skip_line();
					}
				}
			}
			// else
			// {
			// 	while (parse("["))
			// 	{
			// 		skip_line();
			// 	}
			// }
			stream.consume();
			const std::ptrdiff_t fragment_source_begin = stream.pos();
			const std::ptrdiff_t fragment_source_end = find(endoffile);
			if (core::member_table<T>::has("fragment_source"))
			{
				parse_region(fragment_source_begin, fragment_source_end, x.fragment_source);
			}
			else
			{
				skip_region(fragment_source_begin, fragment_source_end);
			}
			// debug_printline(std::string(stream.data(fragment_source_begin), stream.data(fragment_source_end)));

			debug_assert(!stream.valid());
		}
	private:
		template <typename T>
		bool read_name_value(std::vector<T> & x)
		{
			x.emplace_back();

			static_assert(core::member_table<T>::has("name"), "");
			if (!core::member_table<T>::call("name", x.back(), [&](auto & y){ return parse(y); }))
			{
				debug_printline("failed to parse 'name' in '", stream.filepath(), "'");
				return false;
			}

			debug_verify(parse(whitespace), "'name' and 'value' should be separated by whitespace in '", stream.filepath(), "'");

			static_assert(core::member_table<T>::has("value"), "");
			if (!core::member_table<T>::call("value", x.back(), [&](auto & y){ return parse(y); }))
			{
				debug_printline("failed to parse 'value' in '", stream.filepath(), "'");
				return false;
			}

			return true;
		}
		template <typename T>
		bool read_name_value(T &)
		{
			debug_unreachable();
		}

		bool parse_impl() const
		{
			return true;
		}

		template <typename ...Ps>
		bool parse_impl(beginline_t, Ps && ...ps)
		{
			if (!stream.valid())
				return false;

			if (!(stream.pos() <= 0 ||
			      (stream.peek(stream.pos() - 1) == '\n' ||
			       stream.peek(stream.pos() - 1) == '\r')))
				return false;

			return parse_impl(std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(endline_t, Ps && ...ps)
		{
			if (!stream.valid())
				return false;

			while (stream.peek() == '\n' ||
			       stream.peek() == '\r')
			{
				stream.next();
				if (!stream.valid())
					break;
			}

			return parse_impl(std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(newline_t, Ps && ...ps)
		{
			return parse_impl(endline, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(whitespace_t, Ps && ...ps)
		{
			if (!stream.valid())
				return false;

			while (stream.peek() == '\n' ||
			       stream.peek() == '\r' ||
			       stream.peek() == '\t' ||
			       stream.peek() == ' ')
			{
				stream.next();
				if (!stream.valid())
					return false;
			}

			return parse_impl(std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(const char * pattern, Ps && ...ps)
		{
			for (const char * p = pattern; *p != '\0'; p++)
			{
				if (!stream.valid())
					return false;

				if (*p != stream.peek())
					return false;

				stream.next();
				if (!stream.valid())
					return false;
			}

			return parse_impl(std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(int & x, Ps && ...ps)
		{
			if (!stream.valid())
				return false;

			const auto from = stream.pos();
			while (stream.peek() >= '0' && stream.peek() <= '9')
			{
				stream.next();
				if (!stream.valid())
					return false;
			}
			utility::from_string(std::string(stream.data(from), stream.data(stream.pos())), x, true);

			return parse_impl(std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		bool parse_impl(std::string & x, Ps && ...ps)
		{
			if (!stream.valid())
				return false;

			if (stream.peek() == '"')
			{
				debug_fail();
				return false;
			}
			else
			{
				const auto from = stream.pos();
				while (!(stream.peek() == '\n' ||
				         stream.peek() == '\r' ||
				         stream.peek() == '\t' ||
				         stream.peek() == ' '))
				{
					stream.next();
					if (!stream.valid())
						return false;
				}
				x = std::string(stream.data(from), stream.data(stream.pos()));

				return parse_impl(std::forward<Ps>(ps)...);
			}
		}

		template <typename P1, typename ...Ps>
		bool parse(P1 && p1, Ps && ...ps)
		{
			const auto pos = stream.pos();
			if (parse_impl(std::forward<P1>(p1), std::forward<Ps>(ps)...))
				return true;

			stream.seek(pos);
			return false;
		}

		std::ptrdiff_t find(endoffile_t)
		{
			const auto bak = stream.pos();
			while (stream.valid())
			{
				stream.next();
			}
			const auto pos = stream.pos();
			stream.seek(bak);
			return pos;
		}

		template <typename P1, typename ...Ps>
		std::ptrdiff_t find(P1 && p1, Ps && ...ps)
		{
			const auto bak = stream.pos();
			while (stream.valid())
			{
				const auto pos = stream.pos();
				const auto match = parse_impl(p1, ps...);
				stream.seek(pos);

				if (match)
					break;

				stream.next();
			}
			const auto pos = stream.pos();
			stream.seek(bak);
			return pos;
		}

		void parse_region(std::ptrdiff_t from, std::ptrdiff_t to, std::string & x)
		{
			debug_assert(from == stream.pos());

			x = std::string(stream.data(from), stream.data(to));
			stream.seek(to);
		}

		void skip_line()
		{
			while (stream.valid() &&
			       !(stream.peek() == '\n' ||
			         stream.peek() == '\r'))
			{
				stream.next();
			}
			while (stream.valid() &&
			       (stream.peek() == '\n' ||
			        stream.peek() == '\r'))
			{
				stream.next();
			}
		}

		void skip_region(std::ptrdiff_t debug_expression(from), std::ptrdiff_t to)
		{
			debug_assert(from == stream.pos());

			stream.seek(to);
		}
	};
}

#endif /* CORE_SHADERSTRUCTURER_HPP */
