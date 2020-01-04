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
	// C++11 true/false type
	using std::true_type;
	using std::false_type;

	// C++11 integral constant
	using std::integral_constant;
	// C++17 bool constant
	template <bool Value>
	using bool_constant = integral_constant<bool, Value>;

	// C++11 enable_if
	using std::enable_if;
	// C++14 enable if type
	template <bool Cond, typename T = void>
	using enable_if_t = typename enable_if<Cond, T>::type;

	// C++11 conditional
	using std::conditional;
	// C++14 conditional type
	template <bool Cond, typename TrueType, typename FalseType>
	using conditional_t = typename conditional<Cond, TrueType, FalseType>::type;

	// C++11 remove reference
	using std::remove_reference;
	// C++14 remove reference type
	template <typename T>
	using remove_reference_t = typename remove_reference<T>::type;

	// C++11 remove const volatile
	using std::remove_cv;
	// C++14 remove const volatile type
	template <typename T>
	using remove_cv_t = typename remove_cv<T>::type;

	// C++20 remove const volatile referene
	template <typename T>
	struct remove_cvref
	{
		using type = remove_cv_t<remove_reference_t<T>>;
	};
	template <typename T>
	using remove_cvref_t = typename remove_cvref<T>::type;

	// C++11 decay
	using std::decay;
	// C++14 decay type
	template <typename T>
	using decay_t = typename decay<T>::type;

	// C++17 conjunction
	template <typename ...Bs>
	struct conjunction : true_type {};
	template <typename B1, typename ...Bs>
	struct conjunction<B1, Bs...> : conditional_t<bool(B1::value), conjunction<Bs...>, B1> {};
	// C++17 disjunction
	template <typename ...Bs>
	struct disjunction : false_type {};
	template <typename B1, typename ...Bs>
	struct disjunction<B1, Bs...> : conditional_t<bool(B1::value), B1, disjunction<Bs...>> {};
	// C++17 negation
	template <typename B>
	struct negation : bool_constant<!bool(B::value)> {};

	// C++17 void
	template <typename ...>
	struct void_impl
	{
		using type = void;
	};
	template <typename ...Ts>
	using void_t = typename void_impl<Ts...>::type;

	// C++17 add_const
	template <typename T>
	struct add_const
	{
		using type = const T;
	};
	template <typename T>
	struct add_const<const T>
	{
		using type = const T;
	};
	template <typename T>
	using add_const_t = typename add_const<T>::type;

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

	////////////////////////////////////////////////////////////////////////////
	//
	//  std-extensions
	//
	//////////////////////////////////////////////////////////////////
	template <std::size_t Value>
	using index_constant = integral_constant<std::size_t, Value>;

	template <bool Cond, typename T = void>
	using disable_if = enable_if<!Cond, T>;
	template <bool Cond, typename T = void>
	using disable_if_t = typename disable_if<Cond, T>::type;

	template <typename ...Ts>
	struct is_same : is_same<type_list<Ts...>> {};
	template <>
	struct is_same<type_list<>> : true_type {};
	template <typename T1, typename ...Ts>
	struct is_same<type_list<T1, Ts...>> : conjunction<std::is_same<T1, Ts>...> {};

	template <typename ...Ts>
	using is_different = negation<is_same<Ts...>>;

	struct is_brace_constructible_impl
	{
		template <typename T>
		static true_type helper(T &&);

		template <typename T, typename ...Ps>
		static auto test(int) -> decltype(helper(T{std::declval<Ps>()...}));
		template <typename T, typename ...Ps>
		static false_type test(...);
	};
	template <typename T, typename ...Ps>
	using is_brace_constructible = decltype(is_brace_constructible_impl::test<T, Ps...>(0));

	struct is_paren_constructible_impl
	{
		template <typename T>
		static true_type helper(T &&);

		template <typename T, typename ...Ps>
		static auto test(int) -> decltype(helper(T(std::declval<Ps>()...)));
		template <typename T, typename ...Ps>
		static false_type test(...);
	};
	template <typename T, typename ...Ps>
	using is_paren_constructible = decltype(is_paren_constructible_impl::test<T, Ps...>(0));

	namespace is_range_impl
	{
		using std::begin;
		using std::end;

		template <typename T>
		auto test(int) -> decltype(begin(std::declval<T>()), end(std::declval<T>()), true_type());
		template <typename T>
		auto test(...) -> false_type;
	}
	template <typename T>
	using is_range = decltype(is_range_impl::test<T>(0));

	template <bool Cond, typename T>
	using add_const_if = conditional_t<Cond, add_const_t<T>, T>;

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

	template <typename Lout, typename ...List>
	struct concat_impl;
	template <typename Lout>
	struct concat_impl<Lout> : type_is<Lout> {};
	template <typename ...Touts, typename ...Tins, typename ...List>
	struct concat_impl<type_list<Touts...>, type_list<Tins...>, List...> : concat_impl<type_list<Touts..., Tins...>, List...> {};
	template <typename ...Touts, typename Tin, typename ...List>
	struct concat_impl<type_list<Touts...>, Tin, List...> : concat_impl<type_list<Touts..., Tin>, List...> {};
	template <typename ...List>
	using concat = typename concat_impl<type_list<>, List...>::type;

	template <typename ...Ts>
	struct make_type_list_impl : type_is<type_list<Ts...>> {};
	template <typename ...Ts>
	struct make_type_list_impl<type_list<Ts...>> : type_is<type_list<Ts...>> {};
	template <typename ...List>
	using make_type_list = typename make_type_list_impl<List...>::type;

	template <typename L>
	struct last_impl;
	template <typename T>
	struct last_impl<type_list<T>> : type_is<T> {};
	template <typename T, typename ...Ts>
	struct last_impl<type_list<T, Ts...>> : last_impl<type_list<Ts...>> {};
	template <typename ...List>
	using last = typename last_impl<make_type_list<List...>>::type;

	template <std::size_t N, typename L1, typename L2, bool = (N == 0)>
	struct take_impl;
	template <typename L1, typename L2>
	struct take_impl<0, L1, L2, true> : type_is<L2> {};
	template <std::size_t N, typename T, typename ...Ts, typename ...Us>
	struct take_impl<N, type_list<T, Ts...>, type_list<Us...>, false> : take_impl<(N - 1), type_list<Ts...>, type_list<Us..., T>, (N - 1 == 0)> {};
	template <std::size_t N, typename ...List>
	using take = typename take_impl<N, make_type_list<List...>, type_list<>>::type;

	template <template <typename> class F, typename List>
	struct transform_impl;
	template <template <typename> class F, typename ...Ts>
	struct transform_impl<F, type_list<Ts...>> : type_is<type_list<F<Ts>...>> {};
	// F needs to be a type alias
	template <template <typename> class F, typename ...List>
	using transform = typename transform_impl<F, concat<List...>>::type;

	template <typename ...Ts>
	constexpr auto generate_array_impl(type_list<Ts...>)
		-> std::array<typename car<Ts...>::value_type, sizeof...(Ts)>
	{
		return std::array<typename car<Ts...>::value_type, sizeof...(Ts)>{{Ts::value...}};
	}
	template <typename ...List>
	constexpr auto generate_array()
		-> decltype(generate_array_impl(concat<List...>{}))
	{
		return generate_array_impl(concat<List...>{});
	}

	template <template <typename ...> class T, typename L>
	struct apply_impl;
	template <template <typename ...> class T, typename ...Ts>
	struct apply_impl<T, type_list<Ts...>> : type_is<T<Ts...>> {};
	template <template <typename ...> class T, typename ...List>
	using apply = typename apply_impl<T, make_type_list<List...>>::type;

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

	template <typename T, T ...Ns>
	struct integral_max;
	template <typename T, T N>
	struct integral_max<T, N> : integral_constant<T, N> {};
	template <typename T, T N1, T N2, T ...Ns>
	struct integral_max<T, N1, N2, Ns...> : integral_max<T, (N2 < N1 ? N1 : N2), Ns...> {};

	template <typename T>
	struct size_of : index_constant<sizeof(T)> {};
	template <>
	struct size_of<void> : index_constant<0> {};

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

	template <typename ...Ts>
	using make_index_sequence_for = make_index_sequence<sizeof...(Ts)>;

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
		template <typename ...Ps>
		struct best_convertible_calls;
		template <typename P, typename ...Ps>
		struct best_convertible_calls<P, Ps...> : best_convertible_calls<Ps...>
		{
			using best_convertible_calls<Ps...>::call;

			P call(P);
		};
		template <>
		struct best_convertible_calls<>
		{
			void call(...);
		};
	}

	template <typename A, typename ...Ps>
	struct best_convertible
	{
		using type = decltype(detail::best_convertible_calls<Ps...>{}.call(std::declval<A>()));
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

	/////////////////////
	//
	//  stuff
	//
	////////////////
	template <typename T, typename Tuple, typename Indices = mpl::make_tuple_sequence<mpl::remove_cvref_t<Tuple>>>
	struct is_constructible_from_tuple;
	template <typename T, typename Tuple, std::size_t ...Is>
	struct is_constructible_from_tuple<T, Tuple, mpl::index_sequence<Is...>> : std::is_constructible<T, decltype(std::get<Is>(std::declval<Tuple>()))...> {};

	namespace detail
	{
		template <typename T, typename ...Ts>
		struct common_type_unless_nonvoid : mpl::type_is<T> {};
		template <typename ...Ts>
		struct common_type_unless_nonvoid<void, Ts...> : std::common_type<Ts...> {};
	}
	template <typename T, typename ...Ts>
	using common_type_unless_nonvoid = typename detail::common_type_unless_nonvoid<T, Ts...>::type;
}

#endif /* UTILITY_TYPETRAITS_HPP */
