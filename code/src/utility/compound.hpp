#pragma once

#include "utility/algorithm.hpp"
#include "utility/concepts.hpp"
#include "utility/type_traits.hpp"

#include <tuple>
#include <utility>

namespace utility
{
	template <typename ...Rs>
	class compound;

	template <typename ...Ts>
	struct is_compound : mpl::false_type {};
	template <typename ...Rs>
	struct is_compound<compound<Rs...>> : mpl::true_type {};

	namespace detail
	{
		template <std::size_t N>
		struct compound_base_impl
		{
			template <typename ...Rs>
			using type = std::tuple<Rs...>;
		};

		template <>
		struct compound_base_impl<1>
		{
			template <typename R>
			struct type
				: std::tuple<R>
			{
				using this_type = type<R>;
				using base_type = std::tuple<R>;

				type() = default;

				template <typename P,
				          REQUIRES((std::is_constructible<base_type, P>::value))>
				type(P && p) : base_type(std::forward<P>(p)) {}

				template <typename Value>
				this_type & operator = (Value && value)
				{
					std::get<0>(*this) = std::forward<Value>(value);
					return *this;
				}

				operator R & () & { return std::get<0>(*this); }
				operator R && () && { return static_cast<R &&>(std::get<0>(*this)); }
			};
		};

		template <>
		struct compound_base_impl<2>
		{
			template <typename R1, typename R2>
			using type = std::pair<R1, R2>;
		};

		template <typename ...Rs>
		using compound_base = typename compound_base_impl<sizeof...(Rs)>::template type<Rs...>;
	}

	template <typename ...Rs>
	class compound
		: public detail::compound_base<Rs...>
	{
		template <typename ...Rs_>
		friend class compound;

		using this_type = compound<Rs...>;
		using base_type = detail::compound_base<Rs...>;

	public:
		template <typename ...Ps,
		          REQUIRES((!is_compound<mpl::remove_cvref_t<Ps>...>::value)),
		          REQUIRES((!ext::is_tuple<Ps...>::value)),
		          REQUIRES((std::is_constructible<base_type, Ps...>::value))>
		compound(Ps && ...ps)
			: base_type(std::forward<Ps>(ps)...)
		{}

		// todo constructible from tuple
		template <typename Tuple,
		          REQUIRES((ext::tuple_size<Tuple>::value == sizeof...(Rs)))>
		compound(Tuple && tuple)
			: compound(std::forward<Tuple>(tuple), mpl::make_index_sequence<sizeof...(Rs)>{})
		{}

		template <typename Value,
		          REQUIRES((!is_compound<mpl::remove_cvref_t<Value>>::value)),
		          REQUIRES((!ext::is_tuple<Value>::value)),
		          REQUIRES((std::is_assignable<base_type, Value>::value))>
		this_type & operator = (Value && value)
		{
			static_cast<base_type &>(*this) = std::forward<Value>(value);
			return *this;
		}

		// todo assignable from tuple
		template <typename Tuple,
		          REQUIRES((ext::tuple_size<Tuple>::value == sizeof...(Rs)))>
		this_type & operator = (Tuple && tuple)
		{
			assign(std::forward<Tuple>(tuple), mpl::make_index_sequence<sizeof...(Rs)>{});
			return *this;
		}

	private:
		template <typename Tuple, std::size_t ...Is>
		compound(Tuple && tuple, mpl::index_sequence<Is...>)
			: base_type(std::get<Is>(std::forward<Tuple>(tuple))...)
		{}

		template <typename Tuple, std::size_t ...Is>
		void assign(Tuple && tuple, mpl::index_sequence<Is...>)
		{
			int expansion_hack[] = {(std::get<Is>(*this) = std::get<Is>(std::forward<Tuple>(tuple)), 0)...};
			fiw_unused(expansion_hack);
		}
	};

	template <typename ...Ts>
	auto forward_as_compound(Ts && ...ts)
	{
		return compound<Ts &&...>(std::forward<Ts>(ts)...);
	}

	namespace detail
	{
		template <std::size_t N, typename Compound>
		auto last_compound(Compound && compound)
		{
			return ext::apply_for([](auto && ...ps){ return forward_as_compound(std::forward<decltype(ps)>(ps)...); }, std::forward<Compound>(compound), mpl::integral_shift<std::size_t, mpl::make_index_sequence<N>, (ext::tuple_size<Compound>::value - N)>{});
		}
	}

	template <std::size_t N, typename ...Ts>
	auto last(compound<Ts...> & compound) { return detail::last_compound<N>(compound); }
	template <std::size_t N, typename ...Ts>
	auto last(const compound<Ts...> & compound) { return detail::last_compound<N>(compound); }
	template <std::size_t N, typename ...Ts>
	auto last(compound<Ts...> && compound) { return detail::last_compound<N>(std::move(compound)); }
	template <std::size_t N, typename ...Ts>
	auto last(const compound<Ts...> && compound) { return detail::last_compound<N>(std::move(compound)); }

	template <template <typename ...> class P, typename ...Ts>
	using combine = mpl::conditional_t<(mpl::concat<Ts...>::size == 1), mpl::car<Ts...>, mpl::apply<P, Ts...>>;

	template <std::size_t N, typename T,
	          REQUIRES((1 == N))>
	decltype(auto) select_first(T && t)
	{
		return std::forward<T>(t);
	}

	template <std::size_t N, typename T,
	          REQUIRES((1 < N))>
	decltype(auto) select_first(T && t)
	{
		return std::get<0>(std::forward<T>(t));
	}
}
