#pragma once

#include "utility/iterator.hpp"
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

		utility::zip_iterator<Ts *...> address(pointer p, size_type n)
		{
			return utl::unpack(offsets(n), [p](auto ...is){ return utility::zip_iterator<Ts *...>(reinterpret_cast<Ts *>(reinterpret_cast<char *>(p) + is)...); });
		}
		utility::zip_iterator<const Ts *...> address(const_pointer p, size_type n) const
		{
			return utl::unpack(offsets(n), [p](auto ...is){ return utility::zip_iterator<const Ts *...>(reinterpret_cast<const Ts *>(reinterpret_cast<const char *>(p) + is)...); });
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

		template <typename ...Ps>
		void construct(const utility::zip_iterator<Ts *...> & p, Ps && ...ps)
		{
			construct_impl(mpl::make_index_sequence<sizeof...(Ts)>{}, p, std::forward<Ps>(ps)...);
		}
		template <typename ...Ps>
		void construct(const utility::zip_iterator<Ts *...> & p, std::piecewise_construct_t, Ps && ...ps)
		{
			piecewise_construct_impl(mpl::make_index_sequence<sizeof...(Ts)>{}, p, std::forward<Ps>(ps)...);
		}
		void destroy(const utility::zip_iterator<Ts *...> & p)
		{
			utl::unpack(static_cast<const std::tuple<Ts *...> &>(p), [this](auto ...ps){ int expansion_hack[] = {(base().destroy(ps), 0)...}; });
		}

	private:
		base_type & base() { return static_cast<base_type &>(*this); }
		const base_type & base() const { return static_cast<const base_type &>(*this); }

		template <typename ...Ps>
		void construct_impl(mpl::index_sequence<0>, const std::tuple<Ts *...> & ptrs, Ps && ...ps)
		{
			static_assert(sizeof...(Ts) == 1, "");
			construct(std::get<0>(ptrs), std::forward<Ps>(ps)...);
		}
		template <std::size_t ...Is, typename P,
		          REQUIRES((utility::is_proxy_reference<mpl::remove_cvref_t<P>>::value))>
		void construct_impl(mpl::index_sequence<Is...>, const std::tuple<Ts *...> & ptrs, P && p)
		{
			int expansion_hack[] = {(construct(std::get<Is>(ptrs), utility::get<Is>(std::forward<P>(p))), 0)...};
		}
		template <std::size_t ...Is, typename ...Ps,
		          REQUIRES((sizeof...(Ps) != 1 || !utility::is_proxy_reference<mpl::remove_cvref_t<mpl::car<Ps...>>>::value))>
		void construct_impl(mpl::index_sequence<Is...>, const std::tuple<Ts *...> & ptrs, Ps && ...ps)
		{
			int expansion_hack[] = {(construct(std::get<Is>(ptrs), std::forward<Ps>(ps)), 0)...};
		}
		// crashes clang 4.0
		// template <std::size_t ...Is, typename ...Ps>
		// void piecewise_construct_impl(mpl::index_sequence<Is...>, const std::tuple<Ts *...> & p, Ps && ...ps)
		// {
		// 	int expansion_hack[] = {(utl::unpack(std::forward<Ps>(ps), [this, &p](auto && ...ps){ construct(std::get<Is>(p), std::forward<decltype(ps)>(ps)...); }), 0)...};
		// }
		void piecewise_construct_impl(mpl::index_sequence<>, const std::tuple<Ts *...> & /*ptrs*/)
		{}
		template <std::size_t I, typename P1, std::size_t ...Is>
		void construct_impl_helper(mpl::index_constant<I>, const std::tuple<Ts *...> & ptrs, P1 && p1, mpl::index_sequence<Is...>)
		{
			construct(std::get<I>(ptrs), std::get<Is>(std::forward<P1>(p1))...);
		}
		template <std::size_t I, std::size_t ...Is, typename P1, typename ...Ps>
		void piecewise_construct_impl(mpl::index_sequence<I, Is...>, const std::tuple<Ts *...> & ptrs, P1 && p1, Ps && ...ps)
		{
			construct_impl_helper(mpl::index_constant<I>{}, ptrs, std::forward<P1>(p1), mpl::make_index_sequence<std::tuple_size<mpl::remove_cvref_t<P1>>::value>{});
			piecewise_construct_impl(mpl::index_sequence<Is...>{}, ptrs, std::forward<Ps>(ps)...);
		}

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

		template <typename U1, typename ...Is>
		static std::array<std::size_t, sizeof...(Ts)> offsets_impl(std::size_t /*capacity*/, std::size_t acc, mpl::type_list<U1>, Is ...is)
		{
			static_assert(sizeof...(Ts) == sizeof...(Is) + 1, "");
			return std::array<std::size_t, sizeof...(Ts)>{is..., acc};
		}
		template <typename U1, typename U2, typename ...Us, typename ...Is>
		static std::array<std::size_t, sizeof...(Ts)> offsets_impl(std::size_t capacity, std::size_t acc, mpl::type_list<U1, U2, Us...>, Is ...is)
		{
			return offsets_impl(capacity, align_upwards(acc + sizeof(U1) * capacity, sizeof(U2)), mpl::type_list<U2, Us...>{}, is..., acc);
		}
		static std::array<std::size_t, sizeof...(Ts)> offsets(std::size_t capacity)
		{
			using U1 = mpl::car<Ts...>;

			return offsets_impl(capacity, align_upwards(mpl::size_of<Header>::value, sizeof(U1)), mpl::type_list<Ts...>{});
		}
	};
}
