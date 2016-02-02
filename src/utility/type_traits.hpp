/**
 * This file is an extension to the standard <type_traits>.
 */
#ifndef UTILITY_TYPETRAITS_HPP
#define UTILITY_TYPETRAITS_HPP

#include <type_traits>

namespace mpl
{
	////////////////////////////////////////////////////////////////////////////
	//
	//  std-extensions
	//
	//////////////////////////////////////////////////////////////////
	template <bool Value>
	using boolean_constant = std::integral_constant<bool, Value>;

	template <bool Cond, typename T = void>
	using disable_if = std::enable_if<!Cond, T>;
	template <bool Cond, typename T = void>
	using enable_if = std::enable_if<Cond, T>;

	template <bool Cond, typename T = void>
	using disable_if_t = typename mpl::disable_if<Cond, T>::type;
	template <bool Cond, typename T = void>
	using enable_if_t = typename mpl::enable_if<Cond, T>::type;

	template <typename T, typename U>
	using is_different = mpl::boolean_constant<!std::is_same<T, U>::value>;
	template <typename T, typename U>
	using is_same = std::is_same<T, U>;

	////////////////////////////////////////////////////////////////////////////
	//
	//  helper classes
	//
	//////////////////////////////////////////////////////////////////
	template <typename T>
	struct type_is
	{
		using type = T;
	};

	////////////////////////////////////////////////////////////////////////////
	//
	//  queries
	//
	//////////////////////////////////////////////////////////////////
	template <typename T, typename U>
	struct fits_in : std::false_type {};
	template <>
	struct fits_in<float, float> : std::true_type {};
	template <>
	struct fits_in<float, double> : std::true_type {};
	template <>
	struct fits_in<double, double> : std::true_type {};
}

#endif /* UTILITY_TYPETRAITS_HPP */
