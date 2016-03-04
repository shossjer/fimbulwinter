/**
 * This file is an extension to the standard <type_traits>.
 */
#ifndef UTILITY_TYPETRAITS_HPP
#define UTILITY_TYPETRAITS_HPP

#include <array>
#include <tuple>
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
	using enable_if_t = typename enable_if<Cond, T>::type;

	template <bool Cond, typename T = void>
	using disable_if = enable_if<!Cond, T>;
	template <bool Cond, typename T = void>
	using disable_if_t = typename disable_if<Cond, T>::type;

	using std::is_same;
	template <typename T, typename U>
	using is_different = boolean_constant<!is_same<T, U>::value>;

	using std::decay;
	template <typename T>
	using decay_t = typename decay<T>::type;

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

	// integral_enumerate
	template <typename T, T B, T E, T D, typename S, typename = void>
	struct integral_enumerate_impl;
	template <typename T, T B, T E, T D, typename S>
	struct integral_enumerate_impl<T, B, E, D, S, enable_if_t<(B < E)>> : type_is<typename integral_enumerate_impl<T, B + D, E, D, integral_append<T, S, B>>::type> {};
	template <typename T, T B, T E, T D, typename S>
	struct integral_enumerate_impl<T, B, E, D, S, enable_if_t<(B >= E)>> : type_is<S> {};
	template <typename T, T B, T E, T D = 1>
	using integral_enumerate = typename integral_enumerate_impl<T, B, E, D, integral_sequence<T>>::type;

	// make_*_sequence
	template <typename T, T N>
	using make_integral_sequence = integral_enumerate<T, 0, N>;
	template <std::size_t N>
	using make_index_sequence = make_integral_sequence<std::size_t, N>;

	template <typename T>
	using make_tuple_sequence = make_index_sequence<std::tuple_size<T>::value>;
	template <typename T>
	using make_array_sequence = make_tuple_sequence<T>;

	// integral_concat
	template <typename S1, typename S2>
	struct integral_concat_impl;
	template <typename T, T ...N1s, T ...N2s>
	struct integral_concat_impl<integral_sequence<T, N1s...>, integral_sequence<T, N2s...>> : type_is<integral_sequence<T, N1s..., N2s...>> {};
	template <typename S1, typename S2>
	using integral_concat = typename integral_concat_impl<S1, S2>::type;

	// transpose_sequence
	template <typename T, std::size_t R, std::size_t C, std::size_t I, typename S_out>
	struct transpose_sequence : type_is<typename transpose_sequence<T, R, C, I + 1, integral_concat<S_out, integral_enumerate<T, I, R * C, C>>>::type> {};
	template <typename T, std::size_t R, std::size_t C, typename S_out>
	struct transpose_sequence<T, R, C, C, S_out> : type_is<S_out> {};
	template <std::size_t R, std::size_t C>
	using make_transpose_sequence = typename transpose_sequence<std::size_t, R, C, 0, index_sequence<>>::type;

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
