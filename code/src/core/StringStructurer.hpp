
#ifndef CORE_STRINGSTRUCTURER_HPP
#define CORE_STRINGSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialize.hpp"

#include <cstdint>
#include <sstream>

namespace core
{
	class StringStructurer
	{
	private:
		std::istringstream ss;

	public:
		template <typename T>
		void operator () (T & x)
		{
			int c = ss.peek();
			if (c == ' ')
			{
				c = ss.get();
			}

			ss >> x;
		}

		void operator () (list_begin_t)
		{
			int c = ss.get();
			if (c == ' ')
			{
				c = ss.get();
			}
			debug_assert(c == '{');
		}
		void operator () (list_end_t)
		{
			const int c = ss.get();
			debug_assert(c == '}');
		}
		void operator () (list_space_t)
		{
			const int c = ss.get();
			debug_assert(c == ',');
		}

		void operator () (tuple_begin_t)
		{
			int c = ss.get();
			if (c == ' ')
			{
				c = ss.get();
			}
			debug_assert(c == '(');
		}
		void operator () (tuple_end_t)
		{
			const int c = ss.get();
			debug_assert(c == ')');
		}
		void operator () (tuple_space_t)
		{
			const int c = ss.get();
			debug_assert(c == ',');
		}

		void operator () (newline_t)
		{
			const int c = ss.get();
			debug_assert(c == '\n');
		}

	public:
		void set(const char * data, size_t size)
		{
			ss.str(std::string(data, size));
		}
	};
}

#endif /* CORE_STRINGSTRUCTURER_HPP */
