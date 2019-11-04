#pragma once

#include "utility/type_traits.hpp"

#include <cassert>
#include <utility>

namespace utility
{
	template <template <typename> class Allocator, typename Header, typename ...Ts>
	class aggregation_allocator
		: Allocator<char>
	{
	private:
		using base_type = Allocator<char>;

	public:
		using size_type = typename base_type::size_type;
		using difference_type = typename base_type::difference_type;
		using pointer = Header *;
		using const_pointer = const Header *;
		using value_type = Header;

		template <typename Header2, typename ...Us>
		struct rebind { using other = aggregation_allocator<Allocator, Header2, Us...>; };

		using propagate_on_container_move_assignment = typename base_type::propagate_on_container_move_assignment;

		template <std::size_t I,
		          typename T = mpl::type_at<I, Ts...>>
		T * address(pointer p, size_type n)
		{
			return reinterpret_cast<T *>(reinterpret_cast<char *>(p) + offset_of(n, mpl::take<(I + 1), Ts...>{}));
		}
		template <std::size_t I,
		          typename T = mpl::type_at<I, Ts...>>
		const T * address(const_pointer p, size_type n) const
		{
			return reinterpret_cast<const T *>(reinterpret_cast<const char *>(p) + offset_of(n, mpl::take<(I + 1), Ts...>{}));
		}

		pointer allocate(size_type n, const void * hint = nullptr)
		{
			static_assert(mpl::integral_max<std::size_t, 1, alignof(Ts)...>::value <= alignof(std::max_align_t), "operator new only guarantees correct alignment for fundamental types");
			char * const p = base().allocate(size_of(n), hint);
			assert(reinterpret_cast<std::uintptr_t>(p) % alignof(std::max_align_t) == 0);

			return reinterpret_cast<value_type *>(p);
		}
		void deallocate(pointer p, size_type n)
		{
			base().deallocate(reinterpret_cast<char *>(p), size_of(n));
		}

		size_type max_size() const { return (base().max_size() - sizeof(Header)) / mpl::integral_sum<mpl::index_sequence<sizeof(Ts)...>>::value; }

		template <typename ...Ps>
		void construct(pointer p, Ps && ...ps) { base().construct(p, std::forward<Ps>(ps)...); }
		template <typename T, typename ...Ps>
		void construct(T * p, Ps && ...ps) { base().construct(p, std::forward<Ps>(ps)...); }
		void destroy(pointer p) { base().destroy(p); }
		template <typename T>
		void destroy(T * p) { base().destroy(p); }

	private:
		base_type & base() { return static_cast<base_type &>(*this); }
		const base_type & base() const { return static_cast<const base_type &>(*this); }

		static constexpr std::size_t align_upwards(std::size_t n, std::size_t a)
		{
			return (n + a - 1) / a * a;
		}

		template <typename U1>
		static std::size_t offset_of_impl(std::size_t capacity, std::size_t acc, mpl::type_list<U1>)
		{
			return acc;
		}
		template <typename U1, typename U2, typename ...Us>
		static std::size_t offset_of_impl(std::size_t capacity, std::size_t acc, mpl::type_list<U1, U2, Us...>)
		{
			return offset_of_impl(capacity, align_upwards(acc + sizeof(U1) * capacity, sizeof(U2)), mpl::type_list<U2, Us...>{});
		}

		template <typename U1, typename ...Us>
		static std::size_t offset_of(std::size_t capacity, mpl::type_list<U1, Us...>)
		{
			return offset_of_impl(capacity, align_upwards(mpl::size_of<Header>::value, sizeof(U1)), mpl::type_list<U1, Us...>{});
		}

		static std::size_t size_of(std::size_t capacity)
		{
			using TN = mpl::last<Ts...>;

			return offset_of(capacity, mpl::type_list<Ts...>{}) + sizeof(TN) * capacity;
		}
	};
}
