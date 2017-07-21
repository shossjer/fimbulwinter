
#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <utility/concepts.hpp>
#include <utility/type_traits.hpp>

#include <utility>

namespace utility
{
	struct in_place_t
	{
		explicit in_place_t() = default;
	};
	constexpr in_place_t in_place = in_place_t{};

	template <size_t>
	struct in_place_index_helper {};
	template <size_t N>
	in_place_index_helper<N> in_place_index() { return {}; }
	template <size_t N>
	using in_place_index_t = in_place_index_helper<N>();

	template <typename T>
	struct is_in_place_index : mpl::false_type {};
	template <size_t I>
	struct is_in_place_index<in_place_index_t<I>> : mpl::true_type {};

	template <typename T>
	struct in_place_type_helper {};
	template <typename T>
	in_place_type_helper<T> in_place_type() { return {}; }
	template <typename T>
	using in_place_type_t = in_place_type_helper<T>();

	template <typename T>
	struct is_in_place_type : mpl::false_type {};
	template <typename T>
	struct is_in_place_type<in_place_type_t<T>> : mpl::true_type {};

	template <typename T, typename ...Ps,
	          REQUIRES((mpl::is_paren_constructible<T, Ps...>::value))>
	T construct(Ps && ...ps)
	{
		return T(std::forward<Ps>(ps)...);
	}
	template <typename T, typename ...Ps,
	          REQUIRES((!mpl::is_paren_constructible<T, Ps...>::value)),
	          REQUIRES((mpl::is_brace_constructible<T, Ps...>::value))>
	T construct(Ps && ...ps)
	{
		return T{std::forward<Ps>(ps)...};
	}
}

#endif /* UTILITY_UTILITY_HPP */
