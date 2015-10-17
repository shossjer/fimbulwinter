/**
 * This file is an extension to the standard <type_traits>.
 */
#ifndef UTILITY_TYPETRAITS_HPP
#define UTILITY_TYPETRAITS_HPP

#include <type_traits>

namespace mpl
{
	template <bool Cond, typename T = void>
	using disable_if = std::enable_if<!Cond, T>;
	template <bool Cond, typename T = void>
	using enable_if = std::enable_if<Cond, T>;

	template <bool Cond, typename T = void>
	using disable_if_t = typename mpl::disable_if<Cond, T>::type;
	template <bool Cond, typename T = void>
	using enable_if_t = typename mpl::enable_if<Cond, T>::type;
}

#endif /* _UTILITY_TYPETRAITS_ */
