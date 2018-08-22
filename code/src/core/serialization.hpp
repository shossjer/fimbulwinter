
#ifndef CORE_SERIALIZATION_HPP
#define CORE_SERIALIZATION_HPP

#include "core/debug.hpp"

#include "utility/preprocessor.hpp"
#include "utility/string_view.hpp"
#include "utility/type_traits.hpp"

#include "utility/lookup_table.hpp"

namespace core
{
	template <typename T>
	class serialization
	{
	private:
		static constexpr decltype(T::serialization()) member_table = T::serialization();

	public:
		static constexpr bool has(utility::string_view name)
		{
			return member_table.contains(name);
		}

		template <typename X, typename F>
		static decltype(auto) call(utility::string_view name, X && x, F && f)
		{
			switch (member_table.find(name))
			{
#define CASE(n) case (n):	  \
				return call_impl(mpl::index_constant<((n) < member_table.size() ? (n) : std::size_t(-1))>{}, std::forward<X>(x), std::forward<F>(f))

				PP_EXPAND_128(CASE, 0);
#undef CASE
			default:
				return call_impl(mpl::index_constant<std::size_t(-1)>{}, std::forward<X>(x), std::forward<F>(f));
			}
		}

		template <typename X, typename F>
		static decltype(auto) call_with_all_members(X && x, F && f)
		{
			return call_with_all_members_impl(mpl::make_index_sequence<member_table.capacity>{}, std::forward<X>(x), std::forward<F>(f));
		}

		template <typename X, typename F>
		static void for_each_member(X && x, F && f)
		{
			for_each_member_impl(mpl::index_constant<0>{}, std::forward<X>(x), std::forward<F>(f));
		}
	private:
		template <typename X, typename F>
		static decltype(auto) call_impl(mpl::index_constant<std::size_t(-1)>, X && x, F && f)
		{
			debug_unreachable();
			return f(x.* member_table.template get_value<0>());
		}
		template <std::size_t I, typename X, typename F>
		static decltype(auto) call_impl(mpl::index_constant<I>, X && x, F && f)
		{
			return f(x.* member_table.template get_value<I>());
		}

		template <std::size_t ...Is, typename X, typename F>
		static decltype(auto) call_with_all_members_impl(mpl::index_sequence<Is...>, X && x, F && f)
		{
			return f(x.* member_table.template get_value<Is>()...);
		}

		template <typename X, typename F>
		static void for_each_member_impl(mpl::index_constant<member_table.capacity>, X &&, F &&)
		{
		}
		template <std::size_t I, typename X, typename F>
		static void for_each_member_impl(mpl::index_constant<I>, X && x, F && f)
		{
			f(x.* member_table.template get_value<I>());
			for_each_member_impl(mpl::index_constant<I + 1>{}, std::forward<X>(x), std::forward<F>(f));
		}
	};

	template <typename T>
	constexpr decltype(T::serialization()) serialization<T>::member_table;

	template <typename T>
	struct has_member_table_impl
	{
		template <typename U>
		static auto test(int) -> decltype(U::serialization(), mpl::true_type());
		template <typename U>
		static auto test(...) -> mpl::false_type;

		using type = decltype(test<T>(0));
	};

	template <typename T>
	using has_member_table = typename has_member_table_impl<T>::type;

	template <typename T>
	class TryAssign
	{
	private:
		T v;

	public:
		TryAssign(T value)
			: v(std::forward<T>(value))
		{}

	public:
		template <typename U>
		void operator () (U & object)
		{
			impl(object, 0);
		}
	private:
		template <typename U>
		auto impl(U & object, int) -> decltype(object = std::declval<T>(), void())
		{
			object = std::forward<T>(v);
		}
		template <typename U>
		void impl(U & object, ...)
		{
			debug_fail("incompatible types");
		}
	};
}

#endif /* CORE_SERIALIZATION_HPP */
