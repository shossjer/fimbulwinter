
#ifndef UTILITY_HEAP_ALLOCATOR_HPP
#define UTILITY_HEAP_ALLOCATOR_HPP

#include "utility/utility.hpp"

namespace utility
{
	template <typename T>
	class heap_allocator
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
		struct rebind { using other = heap_allocator<U>; };

		using propagate_on_container_move_assignment = std::true_type;

		pointer address(reference x) const { return std::addressof(x); }
		const_pointer address(const_reference x) const { return std::addressof(x); }

		pointer allocate(size_type n, const void * = nullptr)
		{
			if (n > max_size())
				return nullptr;

			static_assert(alignof(T) <= alignof(std::max_align_t), "operator new only guarantees correct alignment for fundamental types");
			return static_cast<T *>(::operator new(n * sizeof(T), std::nothrow));
		}
		void deallocate(pointer p, size_type)
		{
			::operator delete(p, std::nothrow);
		}

		constexpr size_type max_size() const { return size_t(-1) / sizeof(T); }

		template <typename U, typename ...Ps>
		void construct(U * p, Ps && ...ps) { utility::construct_at<U>(p, std::forward<Ps>(ps)...); }
		template <typename U>
		void destroy(U * p) { p->U::~U(); }
	};
}

#endif /* UTILITY_HEAP_ALLOCATOR_HPP */
