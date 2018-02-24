
#ifndef CORE_STRINGSERIALIZER_HPP
#define CORE_STRINGSERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialize.hpp"

#include <cstdint>
#include <sstream>

namespace core
{
	class StringSerializer
	{
	private:
	private:
		std::ostringstream ss;
		bool add_whitespace = false;

		std::string string;

	public:
		template <typename T>
		void operator () (const T & x)
		{
			if (add_whitespace)
			{
				ss.put(' ');
			}

			ss << x;

			add_whitespace = true;
		}

		void operator () (list_begin_t)
		{
			if (add_whitespace)
			{
				ss.put(' ');
			}

			ss.put('{');
			add_whitespace = false;
		}
		void operator () (list_end_t)
		{
			ss.put('}');
			add_whitespace = true;
		}
		void operator () (list_space_t)
		{
			ss.put(',');
			add_whitespace = true;
		}

		void operator () (tuple_begin_t)
		{
			if (add_whitespace)
			{
				ss.put(' ');
			}

			ss.put('(');
			add_whitespace = false;
		}
		void operator () (tuple_end_t)
		{
			ss.put(')');
			add_whitespace = true;
		}
		void operator () (tuple_space_t)
		{
			ss.put(',');
			add_whitespace = true;
		}

		void operator () (newline_t)
		{
			ss.put('\n');
			add_whitespace = false;
		}

	public:
		const char * data() const
		{
			return string.c_str();
		}
		bool empty() const
		{
			return string.empty();
		}
		size_t size() const
		{
			return string.size();
		}

		void clear()
		{
			ss.str("");
		}

		void finalize()
		{
			string = ss.str();
		}
	};
}

#endif /* CORE_STRINGSERIALIZER_HPP */
