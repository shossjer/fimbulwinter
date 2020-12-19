#pragma once

#include "utility/type_traits.hpp"

#include <utility>

namespace ext
{
	namespace detail
	{
		template <typename ...Ts>
		class overload_set;

		template <typename T1, typename T2>
		class overload_set<T1, T2>
			: T1
			, T2
		{
		public:
			template <typename P1, typename P2>
			explicit overload_set(P1 && p1, P2 && p2)
				: T1(std::forward<P1>(p1))
				, T2(std::forward<P2>(p2))
			{}

			using T1::operator ();
			using T2::operator ();
		};

		template <typename T1, typename ...Ts>
		class overload_set<T1, Ts...>
			: T1
			, overload_set<Ts...>
		{
		public:
			template <typename P1, typename ...Ps>
			explicit overload_set(P1 && p1, Ps && ...ps)
				: T1(std::forward<P1>(p1))
				, overload_set<Ts...>(std::forward<Ps>(ps)...)
			{}

			using T1::operator ();
			using overload_set<Ts...>::operator ();
		};
	}

	template <typename P>
	decltype(auto) overload(P && p)
	{
		return std::forward<P>(p);
	}

	template <typename ...Ps>
	decltype(auto) overload(Ps && ...ps)
	{
		return detail::overload_set<mpl::remove_cvref_t<Ps>...>(std::forward<Ps>(ps)...);
	}
}
