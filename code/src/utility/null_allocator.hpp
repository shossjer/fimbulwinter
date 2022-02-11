#pragma once

#include "utility/compiler.hpp"

#include <cstddef>
#include <type_traits>

namespace utility
{
	template <typename T>
	class null_allocator
	{
	public:
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T *;
		using const_pointer = const T *;
		using reference = T &;
		using const_reference = const T &;
		using value_type = T;

		template <typename U>
		struct rebind { using other = null_allocator<U>; };

		using propagate_on_container_move_assignment = std::true_type;

		constexpr pointer address(reference /*x*/) const { return fiw_unreachable(), nullptr; }
		constexpr const_pointer address(const_reference /*x*/) const { return fiw_unreachable(), nullptr; }

		constexpr pointer allocate(size_type /*n*/, const void * /*hint*/ = nullptr) { return nullptr; }
		void deallocate(pointer /*p*/, size_type /*n*/) { fiw_unreachable(); }

		constexpr size_type max_size() const { return 0; }

		template <typename U, typename ...Ps>
		void construct(U * /*p*/, Ps && .../*ps*/) { fiw_unreachable(); }
		template <typename U>
		void destroy(U * /*p*/) { fiw_unreachable(); }
	};
}
