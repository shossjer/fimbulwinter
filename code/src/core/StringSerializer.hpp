
#ifndef CORE_STRINGSERIALIZER_HPP
#define CORE_STRINGSERIALIZER_HPP

#include "core/debug.hpp"
#include "core/serialize.hpp"

#include <cstdint>
#include <sstream>
#include <vector>

namespace core
{
	class StringSerializer
	{
	private:
		struct Data
		{
			enum Type
			{
				CLASS,
				TUPLE
			};

			Type type;
			int next_child;
		};
	private:
		std::ostringstream ss;

		std::vector<Data> stack;

		std::string string;

	public:
		template <typename T>
		void operator () (const T & x)
		{
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					ss.put(',');
					ss.put(' ');
				}
			}

			ss << x;
		}

		template <typename T>
		void push(type_class_t, const T & x)
		{
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					ss.put(',');
					ss.put(' ');
				}
			}
			ss.put('{');

			stack.push_back(Data{Data::CLASS, 0});
		}

		template <typename T>
		void push(type_tuple_t, const T & x)
		{
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					ss.put(',');
					ss.put(' ');
				}
			}
			ss.put('(');

			stack.push_back(Data{Data::TUPLE, 0});
		}

		void pop()
		{
			const auto & top = stack.back();
			if (top.type == Data::CLASS)
			{
				ss.put('}');
			}
			if (top.type == Data::TUPLE)
			{
				ss.put(')');
				ss.put('\n');
			}

			stack.pop_back();
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
