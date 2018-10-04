
#ifndef CORE_SHADERSTRUCTURER_HPP
#define CORE_SHADERSTRUCTURER_HPP

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
		std::vector<char> buffer;
		int read_;

		std::string filename;

	public:
		ShaderStructurer(int size, std::string filename)
			: buffer(size)
			, read_(0)
			, filename(std::move(filename))
		{}
		ShaderStructurer(std::vector<char> && buffer, std::string filename)
			: buffer(std::move(buffer))
			, read_(0)
			, filename(std::move(filename))
		{}

	public:
		char * data() { return buffer.data(); }

		template <typename T>
		void read(T & x)
		{
			debug_verify(parse("[vertex]", newline), "expected vertex section in '", filename, "'");
			static_assert(core::member_table<T>::has("inputs"), "");
			// if (core::member_table<T>::has("inputs"))
			{
				while (parse("[bind", whitespace))
				{
					if (core::member_table<T>::call("inputs", x, [&](auto & y){ return read_name_value(y); }))
					{
						debug_verify(parse("]", newline), "expected ending bracket in '", filename, "'");
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
			const int vertex_source_begin = tell();
			const int vertex_source_end = find(beginline, "[fragment]", endline);
			if (core::member_table<T>::has("vertex_source"))
			{
				parse_region(vertex_source_begin, vertex_source_end, x.vertex_source);
			}
			else
			{
				skip_region(vertex_source_begin, vertex_source_end);
			}

			debug_verify(parse("[fragment]", newline), "expected fragment section in '", filename, "'");
			static_assert(core::member_table<T>::has("outputs"), "");
			// if (core::member_table<T>::has("outputs"))
			{
				while (parse("[bind", whitespace))
				{
					if (core::member_table<T>::call("outputs", x, [&](auto & y){ return read_name_value(y); }))
					{
						debug_verify(parse("]", newline), "expected ending bracket in '", filename, "'");
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
			const int fragment_source_begin = tell();
			const int fragment_source_end = find(endoffile);
			if (core::member_table<T>::has("fragment_source"))
			{
				parse_region(fragment_source_begin, fragment_source_end, x.fragment_source);
			}
			else
			{
				skip_region(fragment_source_begin, fragment_source_end);
			}
		}
	private:
		template <typename T>
		bool read_name_value(std::vector<T> & x)
		{
			x.emplace_back();

			static_assert(core::member_table<T>::has("name"), "");
			if (!core::member_table<T>::call("name", x.back(), [&](auto & y){ return parse(y); }))
			{
				debug_printline("failed to parse 'name' in '", filename, "'");
				return false;
			}

			debug_verify(parse(whitespace), "'name' and 'value' should be separated by whitespace in '", filename, "'");

			static_assert(core::member_table<T>::has("value"), "");
			if (!core::member_table<T>::call("value", x.back(), [&](auto & y){ return parse(y); }))
			{
				debug_printline("failed to parse 'value' in '", filename, "'");
				return false;
			}

			return true;
		}
		template <typename T>
		bool read_name_value(T &)
		{
			debug_unreachable();
		}

		int parse_impl(int curr) const
		{
			return curr;
		}

		template <typename ...Ps>
		int parse_impl(int curr, beginline_t, Ps && ...ps) const
		{
			if (curr == 0)
				return 0;

			if (!(buffer[curr - 1] == '\n' ||
			      buffer[curr - 1] == '\r'))
				return -(curr + 1);

			return parse_impl(curr, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, endline_t, Ps && ...ps) const
		{
			if (curr >= buffer.size())
				return -(curr + 1);

			while (buffer[curr] == '\n' ||
			       buffer[curr] == '\r')
			{
				curr++;
				if (curr >= buffer.size())
					return -(curr + 1);
			}

			return parse_impl(curr, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, newline_t, Ps && ...ps) const
		{
			return parse_impl(curr, endline, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, whitespace_t, Ps && ...ps) const
		{
			if (curr >= buffer.size())
				return -(curr + 1);

			while (buffer[curr] == '\n' ||
			       buffer[curr] == '\r' ||
			       buffer[curr] == '\t' ||
			       buffer[curr] == ' ')
			{
				curr++;
				if (curr >= buffer.size())
					return -(curr + 1);
			}

			return parse_impl(curr, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, const char * pattern, Ps && ...ps) const
		{
			for (const char * p = pattern; *p != '\0'; p++)
			{
				if (curr >= buffer.size())
					return -(curr + 1);

				if (*p != buffer[curr])
					return -(curr + 1);

				curr++;
			}

			return parse_impl(curr, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, int & x, Ps && ...ps) const
		{
			int end = curr;
			if (end >= buffer.size())
				return -(end + 1);

			while (buffer[end] >= '0' && buffer[end] <= '9')
			{
				end++;
				if (end >= buffer.size())
					return -(end + 1);
			}
			utility::from_string(std::string(buffer.data() + curr, buffer.data() + end), x, true);

			return parse_impl(end, std::forward<Ps>(ps)...);
		}

		template <typename ...Ps>
		int parse_impl(int curr, std::string & x, Ps && ...ps) const
		{
			if (curr >= buffer.size())
				return -(curr + 1);

			if (buffer[curr] == '"')
			{
				debug_fail();
				return -(curr + 1);
			}
			else
			{
				int end = curr;
				while (!(buffer[end] == '\n' || buffer[end] == '\r' || buffer[end] == '\t' || buffer[end] == ' '))
				{
					end++;
					if (end >= buffer.size())
						return -(end + 1);
				}
				x = std::string(buffer.data() + curr, buffer.data() + end);

				return parse_impl(end, std::forward<Ps>(ps)...);
			}
		}

		template <typename P1, typename ...Ps>
		bool parse(P1 && p1, Ps && ...ps)
		{
			const int curr = parse_impl(read_, std::forward<P1>(p1), std::forward<Ps>(ps)...);
			if (curr < 0)
				return false;

			read_ = curr;
			return true;
		}

		int tell() const { return read_; }

		int find(endoffile_t) const
		{
			return buffer.size();
		}

		template <typename P1, typename ...Ps>
		int find(P1 && p1, Ps && ...ps) const
		{
			int curr = read_;
			while (true)
			{
				const int match = parse_impl(curr, p1, ps...);
				if (match >= 0)
					return curr;

				if (curr >= buffer.size())
					return -(curr + 1);

				curr++;
			}
		}

		bool parse_region(int from, int to, std::string & x)
		{
			debug_assert(from == read_);
			if (!(from <= to && to <= buffer.size()))
				return false;

			x = std::string(buffer.data() + from, buffer.data() + to);
			read_ = to;
			return true;
		}

		void skip_line()
		{
			int curr = read_;
			while (curr < buffer.size() &&
			       !(buffer[curr] == '\n' ||
			         buffer[curr] == '\r'))
			{
				curr++;
			}
			while (curr < buffer.size() &&
			       (buffer[curr] == '\n' ||
			        buffer[curr] == '\r'))
			{
				curr++;
			}
			read_ = curr;
		}

		void skip_region(int from, int to)
		{
			debug_assert(from == read_);
			if (!(from <= to))
				return;
			read_ = std::min(to, static_cast<int>(buffer.size()));
		}
	};
}

#endif /* CORE_SHADERSTRUCTURER_HPP */
