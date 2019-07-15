
#ifndef UTILITY_TUPLE_HPP
#define UTILITY_TUPLE_HPP

#include "type_traits.hpp"

namespace utility
{
	template <typename ...Ts>
	struct tuple;
	template <typename T, typename ...Ts>
	struct tuple<T, Ts...>
	{
		T head_;
		tuple<Ts...> tail_;

		T & get(mpl::index_constant<0>) { return head_; }
		const T & get(mpl::index_constant<0>) const { return head_; }
		template <std::size_t I>
		decltype(auto) get(mpl::index_constant<I>) { return tail_.get(mpl::index_constant<(I - 1)>{}); }
		template <std::size_t I>
		decltype(auto) get(mpl::index_constant<I>) const { return tail_.get(mpl::index_constant<(I - 1)>{}); }
	};
	template <typename T>
	struct tuple<T>
	{
		T head_;

		T & get(mpl::index_constant<0>) { return head_; }
		const T & get(mpl::index_constant<0>) const { return head_; }
	};
	template <>
	struct tuple<>
	{
	};

	template <std::size_t I, typename ...Ts>
	decltype(auto) get(tuple<Ts...> & x) { return x.get(mpl::index_constant<I>{}); }
	template <std::size_t I, typename ...Ts>
	decltype(auto) get(const tuple<Ts...> & x) { return x.get(mpl::index_constant<I>{}); }
	template <typename T, typename ...Ts>
	decltype(auto) get(tuple<Ts...> & x) { return x.get(typename mpl::index_of<T, Ts...>::type{}); }
	template <typename T, typename ...Ts>
	decltype(auto) get(const tuple<Ts...> & x) { return x.get(typename mpl::index_of<T, Ts...>::type{}); }
}

#endif /* UTILITY_TUPLE_HPP */
