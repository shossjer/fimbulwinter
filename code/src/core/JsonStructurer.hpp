
#ifndef CORE_JSONSTRUCTURER_HPP
#define CORE_JSONSTRUCTURER_HPP

#include "core/debug.hpp"
#include "core/serialize.hpp"

#include "utility/json.hpp"

#include <cstdint>
#include <vector>

namespace core
{
	class JsonStructurer
	{
	private:
		struct Data
		{
			json * current;
			int next_child;
		};
	private:
		json root;

		std::vector<Data> stack;

	public:
		template <typename T>
		void operator () (const char * key, T & x)
		{
			debug_assert(!stack.empty());

			auto & top = stack.back();
			debug_assert(top.current->is_object());
			auto maybe = top.current->find(key);
			debug_assert(maybe != top.current->end());

			x = *maybe;
		}

		template <typename T>
		void push(type_class_t, T & x)
		{
			if (stack.empty())
			{
				stack.push_back(Data{&root, -1});
			}
			else
			{
				auto & top = stack.back();
				debug_assert(top.current->is_array());
				stack.push_back(Data{&(*top.current)[top.next_child++], -1});
			}
		}

		template <typename T>
		void push(type_list_t, const char * key, T & x)
		{
			debug_assert(!stack.empty());

			const auto & top = stack.back();
			auto maybe = top.current->find(key);
			debug_assert(maybe != top.current->end());
			debug_assert(maybe->is_array());

			x.resize(maybe->size());

			stack.push_back(Data{&*maybe, 0});
		}

		void pop()
		{
			stack.pop_back();
		}

	public:
		void set(const char * data, size_t size)
		{
			root = json::parse(data, data + size);
		}
	};
}

#endif /* CORE_JSONSTRUCTURER_HPP */
