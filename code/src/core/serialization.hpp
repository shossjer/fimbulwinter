
#ifndef CORE_SERIALIZATION_HPP
#define CORE_SERIALIZATION_HPP

#include "core/debug.hpp"

#include "utility/preprocessor.hpp"
#include "utility/string_view.hpp"
#include "utility/type_traits.hpp"
#include "utility/utility.hpp"

#include "utility/lookup_table.hpp"

namespace core
{
	template <typename T>
	constexpr auto serialization(utility::in_place_type_t<T>) -> decltype(T::serialization())
	{
		return T::serialization();
	}

	template <typename T>
	class value_table
	{
	private:
		static constexpr decltype(serialization(utility::in_place_type<T>)) lookup_table = serialization(utility::in_place_type<T>);

		static_assert(lookup_table.all_values_same_type, "");

	public:
		static constexpr bool has(utility::string_view name)
		{
			return lookup_table.contains(name);
		}

		static constexpr decltype(auto) get(utility::string_view name)
		{
			return lookup_table.get_value(lookup_table.find(name));
		}

		template <typename P>
		static constexpr decltype(auto) get_key(P && value)
		{
			return lookup_table.get_key(lookup_table.find_value(std::forward<P>(value)));
		}
	};

	template <typename T>
	constexpr decltype(serialization(utility::in_place_type<T>)) value_table<T>::lookup_table;

	template <typename T>
	class member_table
	{
	private:
		using lookup_table_t = decltype(serialization(utility::in_place_type<T>));

	private:
		static constexpr lookup_table_t lookup_table = serialization(utility::in_place_type<T>);

	public:
		static constexpr bool has(utility::string_view name)
		{
			return lookup_table.contains(name);
		}

		static constexpr std::size_t find(utility::string_view name)
		{
			return lookup_table.find(name);
		}

		template <typename X, typename F>
		static decltype(auto) call(utility::string_view name, X && x, F && f)
		{
			switch (lookup_table.find(name))
			{
#define CASE(n) case (n):	  \
				return call_impl(mpl::index_constant<((n) < lookup_table.size() ? (n) : std::size_t(-1))>{}, std::forward<X>(x), std::forward<F>(f))

				PP_EXPAND_128(CASE, 0);
#undef CASE
			default:
				return call_impl(mpl::index_constant<std::size_t(-1)>{}, std::forward<X>(x), std::forward<F>(f));
			}
		}

		template <std::size_t I, typename X>
		static decltype(auto) get(X && x)
		{
			return x.* lookup_table.template get_value<I>();
		}

		template <typename X, typename F>
		static decltype(auto) call_with_all_members(X && x, F && f)
		{
			return call_with_all_members_impl(mpl::make_index_sequence<lookup_table.capacity>{}, std::forward<X>(x), std::forward<F>(f));
		}

		template <typename X, typename F>
		static void for_each_member(X && x, F && f)
		{
			for_each_member_impl(mpl::index_constant<0>{}, std::forward<X>(x), std::forward<F>(f));
		}
	private:
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
		// C4702 - unreachable code
#endif
		template <typename X, typename F>
		static decltype(auto) call_impl(mpl::index_constant<std::size_t(-1)>, X && x, F && f)
		{
			debug_unreachable();
			return f(x.* lookup_table.template get_value<0>());
		}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif
		template <std::size_t I, typename X, typename F>
		static decltype(auto) call_impl(mpl::index_constant<I>, X && x, F && f)
		{
			return f(x.* lookup_table.template get_value<I>());
		}

		template <std::size_t ...Is, typename X, typename F>
		static decltype(auto) call_with_all_members_impl(mpl::index_sequence<Is...>, X && x, F && f)
		{
			return f(x.* lookup_table.template get_value<Is>()...);
		}

		template <typename X, typename F>
		static void for_each_member_impl(mpl::index_constant<lookup_table.capacity>, X &&, F &&)
		{
		}
		template <std::size_t I, typename X, typename F>
		static void for_each_member_impl(mpl::index_constant<I>, X && x, F && f)
		{
			call_f(mpl::index_constant<I>{}, x, f);
			for_each_member_impl(mpl::index_constant<I + 1>{}, std::forward<X>(x), std::forward<F>(f));
		}

		template <std::size_t I, typename X, typename F>
		static auto call_f(mpl::index_constant<I>, X && x, F && f) -> decltype(f(x.* std::declval<lookup_table_t>().template get_value<I>()), void())
		{
			f(x.* lookup_table.template get_value<I>());
		}
		template <std::size_t I, typename X, typename F>
		static auto call_f(mpl::index_constant<I>, X && x, F && f) -> decltype(f(std::declval<lookup_table_t>().template get_key<I>(), x.* std::declval<lookup_table_t>().template get_value<I>()), void())
		{
			f(lookup_table.template get_key<I>(), x.* lookup_table.template get_value<I>());
		}
	};

	template <typename T>
	constexpr decltype(serialization(utility::in_place_type<T>)) member_table<T>::lookup_table;

	template <typename T>
	struct has_lookup_table_impl
	{
		template <typename U>
		static auto test(int) -> decltype(serialization(utility::in_place_type<U>), mpl::true_type());
		template <typename U>
		static auto test(...) -> mpl::false_type;

		using type = decltype(test<T>(0));
	};

	template <typename T>
	using has_lookup_table = typename has_lookup_table_impl<T>::type;

	// todo separate utilities from customization points; `try_assign` is a utility, `for_each` is a customization point
	template <typename T, typename F>
	auto for_each(T & x, std::size_t count, F && f)
		-> decltype(x.resize(count), bool())
	{
		x.resize(count);

		for (auto && y : x)
		{
			if (!f(y))
				return false;
		}
		return true;
	}

	template <std::size_t I, typename T, typename F>
	void assign(T & x, F && f)
	{
		member_table<T>::template get<I>(x) = std::forward<F>(f)();
	}

	namespace detail
	{
		template <typename T, typename F>
		bool try_assign_impl(mpl::index_constant<std::size_t(-1)>, T &, F &&) { return false; }
		template <std::size_t I, typename T, typename F>
		bool try_assign_impl(mpl::index_constant<I>, T & x, F && f)
		{
			assign<I>(x, std::forward<F>(f));

			return true;
		}
	}
	template <std::size_t I, typename T, typename F>
	bool try_assign(T & x, F && f) { return detail::try_assign_impl(mpl::index_constant<I>{}, x, std::forward<F>(f)); }
}

#endif /* CORE_SERIALIZATION_HPP */
