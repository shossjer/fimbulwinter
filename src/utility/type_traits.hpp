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
	//  std
	//
	//////////////////////////////////////////////////////////////////
	// true/false type
	using std::true_type;
	using std::false_type;

	// integral_constant
	using std::integral_constant;

	template <bool Value>
	using bool_constant = integral_constant<bool, Value>;
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

	// conditional
	template <bool Cond, typename TrueType, typename FalseType>
	using conditional_t = typename std::conditional<Cond, TrueType, FalseType>::type;

	using std::remove_reference;
	template <typename T>
	using remove_reference_t = typename remove_reference<T>::type;

	using std::decay;
	template <typename T>
	using decay_t = typename decay<T>::type;

	// align
	template <std::size_t Len, std::size_t Align>
	using aligned_storage_t = typename std::aligned_storage<Len, Align>::type;

	// conjunction
	template <typename ...Bs>
	struct conjunction : true_type {};
	template <typename B1, typename ...Bs>
	struct conjunction<B1, Bs...> : conditional_t<bool(B1::value), conjunction<Bs...>, B1> {};
	// disjunction
	template <typename ...Bs>
	struct disjunction : false_type {};
	template <typename B1, typename ...Bs>
	struct disjunction<B1, Bs...> : conditional_t<bool(B1::value), B1, disjunction<Bs...>> {};
	// negation
	template <typename B>
	struct negation : bool_constant<!bool(B::value)> {};

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

	template <typename ...Ts>
	struct type_list
	{
		enum { size = sizeof...(Ts) };
	};

	template <typename ...>
	struct void_impl : type_is<void> {};
	template <typename ...Ts>
	using void_t = typename void_impl<Ts...>::type;

	////////////////////////////////////////////////////////////////////////////
	//
	//  std-extensions
	//
	//////////////////////////////////////////////////////////////////
	template <typename ...Ts>
	struct is_same : is_same<type_list<Ts...>> {};
	template <>
	struct is_same<type_list<>> : true_type {};
	template <typename T1, typename ...Ts>
	struct is_same<type_list<T1, Ts...>> : conjunction<std::is_same<T1, Ts>...> {};

	template <typename ...Ts>
	using is_different = negation<is_same<Ts...>>;

	////////////////////////////////////////////////////////////////////////////
	//
	//  lists
	//
	//////////////////////////////////////////////////////////////////
	template <typename ...Ts>
	struct car_impl : car_impl<type_list<Ts...>> {};
	template <typename T1, typename ...Ts>
	struct car_impl<type_list<T1, Ts...>> : type_is<T1> {};
	template <typename ...Ts>
	using car = typename car_impl<Ts...>::type;

	template <typename ...Ts>
	struct cdr_impl : cdr_impl<type_list<Ts...>> {};
	template <typename T1, typename ...Ts>
	struct cdr_impl<type_list<T1, Ts...>> : type_is<type_list<Ts...>> {};
	template <typename ...Ts>
	using cdr = typename cdr_impl<Ts...>::type;

	template <std::size_t N, typename ...Ts>
	struct type_at_impl : type_at_impl<N, type_list<Ts...>> {};
	template <std::size_t N, typename T, typename ...Ts>
	struct type_at_impl<N, type_list<T, Ts...>> : type_at_impl<(N - 1), type_list<Ts...>> {};
	template <typename T, typename ...Ts>
	struct type_at_impl<0, type_list<T, Ts...>> : type_is<T> {};
	template <std::size_t N, typename ...List>
	using type_at = typename type_at_impl<N, List...>::type;

	template <bool C, std::size_t N, typename List, typename Default>
	struct type_at_or_impl : type_is<Default> {};
	template <std::size_t N, typename List, typename Default>
	struct type_at_or_impl<true, N, List, Default> : type_at_impl<N, List> {};
	template <std::size_t N, typename List, typename Default = void>
	using type_at_or = typename type_at_or_impl<(N < List::size), N, List, Default>::type;

	template <template <typename ...> class P, typename Lin, typename Lout, typename ...Args>
	struct type_filter_impl;
	template <template <typename ...> class P, typename Lout, typename ...Args>
	struct type_filter_impl<P, type_list<>, Lout, Args...> : type_is<Lout> {};
	template <template <typename ...> class P, typename T, typename ...Tins, typename ...Touts, typename ...Args>
	struct type_filter_impl<P, type_list<T, Tins...>, type_list<Touts...>, Args...> : type_filter_impl<P, type_list<Tins...>, conditional_t<P<T, Args...>::value, type_list<T, Touts...>, type_list<Touts...>>, Args...> {};
	template <template <typename ...> class P, typename List, typename ...Args>
	using type_filter = typename type_filter_impl<P, List, type_list<>, Args...>::type;

	template <typename T, typename ...Ts>
	struct index_of : index_of<T, type_list<Ts...>> {};
	template <typename T, typename ...Ts>
	struct index_of<T, type_list<T, Ts...>> :
		index_constant<0> {};
	template <typename T, typename T1, typename ...Ts>
	struct index_of<T, type_list<T1, Ts...>> :
		index_constant<index_of<T, type_list<Ts...>>::value + 1> {};

	template <typename T, typename ...Ts>
	struct member_of : member_of<T, type_list<Ts...>> {};
	template <typename T, typename ...Ts>
	struct member_of<T, type_list<Ts...>> :
		disjunction<is_same<T, Ts>...> {};

	////////////////////////////////////////////////////////////////////////////
	//
	//  sequences
	//
	//////////////////////////////////////////////////////////////////
	// integral_sequence
	template <typename T, T ...Ns>
	struct integral_sequence
	{
		enum { size = sizeof...(Ns) };
	};

	template <std::size_t ...Ns>
	using index_sequence = integral_sequence<std::size_t, Ns...>;

	//
	template <typename S, typename U>
	struct integral_convert_impl;
	template <typename T, T ...Ns, typename U>
	struct integral_convert_impl<integral_sequence<T, Ns...>, U> : type_is<integral_sequence<U, U(Ns)...>> {};
	template <typename S, typename T>
	using integral_convert = typename integral_convert_impl<S, T>::type;

	// integral_concat
	template <typename S1, typename S2>
	struct integral_concat_impl;
	template <typename T, T ...N1s, T ...N2s>
	struct integral_concat_impl<integral_sequence<T, N1s...>, integral_sequence<T, N2s...>> : type_is<integral_sequence<T, N1s..., N2s...>> {};
	template <typename S1, typename S2>
	using integral_concat = typename integral_concat_impl<S1, S2>::type;

	// integral_scale
	template <typename T, typename S, T Amount>
	struct integral_scale_impl;
	template <typename T, T ...Ns, T Amount>
	struct integral_scale_impl<T, integral_sequence<T, Ns...>, Amount> : type_is<integral_sequence<T, (Ns * Amount)...>> {};
	template <typename T, typename S, T Amount>
	using integral_scale = typename integral_scale_impl<T, S, Amount>::type;
	// integral_shift
	template <typename T, typename S, T Amount>
	struct integral_shift_impl;
	template <typename T, T ...Ns, T Amount>
	struct integral_shift_impl<T, integral_sequence<T, Ns...>, Amount> : type_is<integral_sequence<T, (Ns + Amount)...>> {};
	template <typename T, typename S, T Amount>
	using integral_shift = typename integral_shift_impl<T, S, Amount>::type;
	// integral_mod
	template <typename T, typename S, T Amount>
	struct integral_mod_impl;
	template <typename T, T ...Ns, T Amount>
	struct integral_mod_impl<T, integral_sequence<T, Ns...>, Amount> : type_is<integral_sequence<T, (Ns % Amount)...>> {};
	template <typename T, typename S, T Amount>
	using integral_mod = typename integral_mod_impl<T, S, Amount>::type;

	//
	template <std::size_t ...Is>
	struct make_index_sequence_impl;
	template <std::size_t N>
	struct make_index_sequence_impl<N> : type_is<integral_concat<typename make_index_sequence_impl<(N / 2)>::type, integral_shift<std::size_t, typename make_index_sequence_impl<(N - (N / 2))>::type, (N / 2)>>> {};
	template <>
	struct make_index_sequence_impl<0> : type_is<index_sequence<>> {};
	template <>
	struct make_index_sequence_impl<1> : type_is<index_sequence<0>> {};
	template <std::size_t N>
	using make_index_sequence = typename make_index_sequence_impl<N>::type;

	template <typename T, T N>
	using make_integral_sequence = integral_convert<make_index_sequence<N>, T>;

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
	template <typename T, T B, T E, T D = 1>
	using integral_enumerate = integral_shift<T, integral_scale<T, make_integral_sequence<T, ((E - B) / D)>, D>, B>;

	// make_*_sequence
	template <typename T>
	using make_tuple_sequence = make_index_sequence<std::tuple_size<T>::value>;
	template <typename T>
	using make_array_sequence = make_tuple_sequence<T>;

	/** transpose_sequence
	 * \tparam R Number of rows in resulting matrix, or number of columns in existing matrix.
	 * \tparam C Number of columns in resulting matrix, or number of rows in existing matrix.
	 */
	template <std::size_t R, std::size_t C>
	using make_transpose_sequence = integral_append<std::size_t, integral_mod<std::size_t, integral_scale<std::size_t, make_index_sequence<(R * C - 1)>, R>, (R * C - 1)>, (R * C - 1)>;

	// what follows is an example of how make_transpose_sequence works:
	// given:
	// 0 1 2 3
	// 4 5 6 7
	// we reshape and compute:
	//        x4   mod7
	// 0 1   0  4  0 4
	// 2 3   8 12  1 5
	// 4 5  16 20  2 6
	// 6 7  24 28  3 0 <-- replace ending zero with 7

	// sum
	template <typename S>
	struct integral_sum;
	template <typename T>
	struct integral_sum<integral_sequence<T>> : integral_constant<T, 0> {};
	template <typename T, T N1>
	struct integral_sum<integral_sequence<T, N1>> : integral_constant<T, N1> {};
	template <typename T, T N1, T N2, T ...Ns>
	struct integral_sum<integral_sequence<T, N1, N2, Ns...>> : integral_sum<integral_sequence<T, (N1 + N2), Ns...>> {};

	////////////////////////////////////////////////////////////////////////////
	//
	//  best convertible
	//
	//////////////////////////////////////////////////////////////////
	namespace detail
	{
		struct best_convertible_dummy {};

		template <typename A, typename P1, typename P2>
		struct best_convertible_helper
		{
			struct convertible_test
			{
				P1 func(P1);
				P2 func(P2);
				best_convertible_dummy func(...);
			};

			using type = decltype(convertible_test{}.func(std::declval<A>()));
		};
		template <typename A, typename P>
		struct best_convertible_helper<A, P, P> : best_convertible_helper<A, best_convertible_dummy, P> {};

		template <typename A, typename ...Ps>
		struct best_convertible_impl;
		template <typename A, typename P1, typename P2, typename ...Ps>
		struct best_convertible_impl<A, P1, P2, Ps...> :
			best_convertible_impl<A, typename best_convertible_helper<A, P1, P2>::type, Ps...> {};
		template <typename A, typename P1, typename P2>
		struct best_convertible_impl<A, P1, P2> :
			best_convertible_helper<A, P1, P2> {};
		template <typename A, typename P1>
		struct best_convertible_impl<A, P1, P1> :
			best_convertible_helper<A, P1, P1> {};
	}

	template <typename A, typename ...Ps>
	class best_convertible
	{
		using best_match = typename detail::best_convertible_impl<A, Ps...>::type;
	public:
		using type = conditional_t<is_same<best_match,
		                                   detail::best_convertible_dummy>::value,
		                           void,
		                           best_match>;
	};
	template <typename A, typename ...Ps>
	using best_convertible_t = typename best_convertible<A, Ps...>::type;

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
