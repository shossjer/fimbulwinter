
#ifndef CORE_STRINGSTRUCTURER_HPP
#define CORE_STRINGSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialize.hpp"

#include <cstdint>
#include <sstream>
#include <vector>

namespace core
{
	class StringStructurer
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
		std::istringstream ss;

		std::vector<Data> stack;

	public:
		template <typename T>
		void operator () (T & x)
		{
			skip_whitespace();
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					int c = ss.get();
					debug_assert(c == ',');
					skip_whitespace();
				}
			}

			ss >> x;
		}

		template <typename T>
		void push(type_class_t, T & x)
		{
			skip_whitespace();
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					int c = ss.get();
					debug_assert(c == ',');
					skip_whitespace();
				}
			}
			int c = ss.get();
			debug_assert(c == '{');

			stack.push_back(Data{Data::CLASS, 0});
		}

		template <typename T>
		void push(type_tuple_t, T & x)
		{
			skip_whitespace();
			if (!stack.empty())
			{
				auto & top = stack.back();
				if (top.next_child++ > 0)
				{
					int c = ss.get();
					debug_assert(c == ',');
					skip_whitespace();
				}
			}
			int c = ss.get();
			debug_assert(c == '(');

			stack.push_back(Data{Data::TUPLE, 0});
		}

		void pop()
		{
			const auto & top = stack.back();
			if (top.type == Data::CLASS)
			{
				skip_whitespace();
				int c = ss.get();
				debug_assert(c == '}');
			}
			if (top.type == Data::TUPLE)
			{
				skip_whitespace();
				int c = ss.get();
				debug_assert(c == ')');
			}

			stack.pop_back();
		}
	private:
		void skip_whitespace()
		{
			for (int c = ss.peek(); c == ' ' || c == '\n'; ss.get(), c = ss.peek());
		}

	public:
		void set(const char * data, size_t size)
		{
			ss.str(std::string(data, size));
		}
	};
}

#endif /* CORE_STRINGSTRUCTURER_HPP */
