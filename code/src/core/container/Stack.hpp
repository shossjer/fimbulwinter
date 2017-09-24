
#ifndef CORE_CONTAINER_STACK_HPP
#define CORE_CONTAINER_STACK_HPP

// #include <array>
#include <stack>

namespace core
{
	namespace container
	{
		// TODO: wanted to use something like std::array, but could not since it
		//       does not support emplace_back and possibly something else
		//       made it therefore temporarily into a vector
		template <typename T, std::size_t N>
		// using Stack = std::stack<T, std::array<T, N>>;
		using Stack = std::stack<T, std::vector<T>>;
	}
}

#endif /* CORE_CONTAINER_STACK_HPP */
