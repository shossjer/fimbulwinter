#pragma once

#include "utility/intrinsics.hpp"
#include "utility/type_traits.hpp"

namespace utility
{
	namespace detail
	{
		struct any_type
		{
			template <typename T>
			operator T ();
		};

		template <typename T, std::size_t ...Ns>
		static auto member_count(mpl::index_sequence<Ns...>, decltype(T{(Ns, any_type{})...}, int())) -> mpl::index_constant<sizeof...(Ns)> { intrinsic_unreachable(); }
		template <typename T, std::size_t N0, std::size_t ...Ns>
		static auto member_count(mpl::index_sequence<N0, Ns...>, ...) { return member_count<T>(mpl::index_sequence<Ns...>{}, 0); }
	}

	template <typename T>
	using member_count = decltype(detail::member_count<T>(mpl::make_index_sequence<(std::is_empty<T>::value ? 0 : sizeof(T))>{}, 0));

#if (defined(_MSVC_LANG) && 201703L <=_MSVC_LANG) || 201703L <= __cplusplus
	namespace detail
	{
		template <typename T>
		auto member_types(mpl::index_constant<0>, T &&) { return mpl::type_list<>{}; }
		template <typename T>
		auto member_types(mpl::index_constant<1>, T && x) { auto [a] = x; return mpl::type_list<decltype(a)>{}; }
		template <typename T>
		auto member_types(mpl::index_constant<2>, T && x) { auto [a, b] = x; return mpl::type_list<decltype(a), decltype(b)>{}; }
		template <typename T>
		auto member_types(mpl::index_constant<3>, T && x) { auto [a, b, c] = x; return mpl::type_list<decltype(a), decltype(b), decltype(c)>{}; }
		template <typename T>
		auto member_types(mpl::index_constant<4>, T && x) { auto [a, b, c, d] = x; return mpl::type_list<decltype(a), decltype(b), decltype(c), decltype(d)>{}; }
		template <typename T>
		auto member_types(mpl::index_constant<5>, T && x)
		{
			auto [a, b, c, d, e] = x;
			return mpl::type_list<decltype(a), decltype(b), decltype(c), decltype(d), decltype(e)>{};
		}
		template <typename T>
		auto member_types(mpl::index_constant<6>, T && x)
		{
			auto [a, b, c, d, e, f] = x;
			return mpl::type_list<decltype(a), decltype(b), decltype(c), decltype(d), decltype(e), decltype(f)>{};
		}
		template <typename T>
		auto member_types(mpl::index_constant<7>, T && x)
		{
			auto [a, b, c, d, e, f, g] = x;
			return mpl::type_list<decltype(a), decltype(b), decltype(c), decltype(d), decltype(e), decltype(f), decltype(g)>{};
		}
		template <typename T>
		auto member_types(mpl::index_constant<8>, T && x)
		{
			auto [a, b, c, d, e, f, g, h] = x;
			return mpl::type_list<decltype(a), decltype(b), decltype(c), decltype(d), decltype(e), decltype(f), decltype(g), decltype(h)>{};
		}
	}

	template <typename T>
	using member_types = decltype(detail::member_types(member_count<T>{}, std::declval<T>()));
#endif
}
