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
	// integral_constant
	using std::integral_constant;

	template <bool Value>
	using boolean_constant = integral_constant<bool, Value>;
	template <std::size_t Value>
	using index_constant = integral_constant<std::size_t, Value>;

	// enable_if
	using std::enable_if;

	template <bool Cond, typename T = void>
	using disable_if = enable_if<!Cond, T>;

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
	//  lists
	//
	//////////////////////////////////////////////////////////////////
	template <typename ...Ts>
	struct type_list {};

	////////////////////////////////////////////////////////////////////////////
	//
	//  sequences
	//
	//////////////////////////////////////////////////////////////////
	// integral_sequence
	template <typename T, T ...Ns>
	struct integral_sequence {};

	template <std::size_t ...Ns>
	using index_sequence = integral_sequence<std::size_t, Ns...>;

	// integral_head
	template <typename T, typename S>
	struct integral_head_impl;
	template <typename T, T N, T ...Ns>
	struct integral_head_impl<T, integral_sequence<T, N, Ns...>> : type_is<integral_constant<T, N>> {};
	template <typename T, typename S>
	using integral_head = typename integral_head_impl<T, S>::type;

	// integral_tail
	template <typename T, typename S>
	struct integral_tail_impl;
	template <typename T, T N, T ...Ns>
	struct integral_tail_impl<T, integral_sequence<T, N, Ns...>> : type_is<integral_sequence<T, Ns...>> {};
	template <typename T, typename S>
	using integral_tail = typename integral_tail_impl<T, S>::type;

	// integral_select
	template <typename T, typename S, std::size_t I>
	struct integral_select_impl : type_is<typename integral_select_impl<T, integral_tail<T, S>, I - 1>::type> {};
	template <typename T, typename S>
	struct integral_select_impl<T, S, 0> : type_is<integral_head<T, S>> {};
	template <typename T, typename S, std::size_t I>
	using integral_select = typename integral_select_impl<T, S, I>::type;

	// integral_append
	template <typename T, typename S, T ...Ns>
	struct integral_append_impl;
	template <typename T, T ...Ms, T ...Ns>
	struct integral_append_impl<T, integral_sequence<T, Ms...>, Ns...> : type_is<integral_sequence<T, Ms..., Ns...>> {};
	template <typename T, typename S, T ...Ns>
	using integral_append = typename integral_append_impl<T, S, Ns...>::type;

	// integral_subsequence
	template <typename T, typename S_in, std::size_t B, std::size_t E, typename S_out>
	struct integral_subsequence_impl : type_is<typename integral_subsequence_impl<T, integral_tail<T, S_in>, B - 1, E - 1, S_out>::type> {};
	template <typename T, typename S_in, std::size_t E, typename S_out>
	struct integral_subsequence_impl<T, S_in, 0, E, S_out> : type_is<typename integral_subsequence_impl<T, integral_tail<T, S_in>, 0, E - 1, integral_append<T, S_out, integral_head<T, S_in>::value>>::type> {};
	template <typename T, typename S_in, typename S_out>
	struct integral_subsequence_impl<T, S_in, 0, 0, S_out> : type_is<S_out> {};
	template <typename T, typename S, std::size_t B, std::size_t E>
	using integral_subsequence = typename integral_subsequence_impl<T, S, B, E, integral_sequence<T>>::type;

	// integral_take
	template <typename T, typename S, std::size_t N>
	using integral_take = integral_subsequence<T, S, 0, N>;

	// enumerate
	template <typename T, T B, T E, typename S>
	struct enumerate_impl : type_is<typename enumerate_impl<T, B + 1, E, integral_append<T, S, B>>::type> {};
	template <typename T, T E, typename S>
	struct enumerate_impl<T, E, E, S> : type_is<S> {};
	template <typename T, T B, T E>
	using enumerate = typename enumerate_impl<T, B, E, integral_sequence<T>>::type;

	// make_*_sequence
	template <typename T, T N>
	using make_integral_sequence = enumerate<T, 0, N>;
	template <std::size_t N>
	using make_index_sequence = make_integral_sequence<std::size_t, N>;

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
