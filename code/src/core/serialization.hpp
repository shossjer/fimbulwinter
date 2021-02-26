#pragma once

#include "core/debug.hpp"

#include "utility/charconv.hpp"
#include "utility/lookup_table.hpp"
#include "utility/optional.hpp"
#include "utility/preprocessor/expand.hpp"
#include "utility/type_traits.hpp"
#include "utility/unicode/string_view.hpp"
#include "utility/utility.hpp"

namespace core
{
	namespace detail
	{
		template <typename X, typename V>
		auto get_member(X && x, V && v)
			-> decltype(v.get(std::forward<X>(x)))
		{
			return v.get(std::forward<X>(x));
		}

		template <typename X, typename V>
		auto get_member(X && x, V && v)
			-> decltype(x.*v)
		{
			return x.*v;
		}
	}

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
		static constexpr bool has(utility::string_units_utf8 name)
		{
			return lookup_table.contains(name);
		}

		static constexpr std::size_t find(utility::string_units_utf8 name)
		{
			return lookup_table.find(name);
		}

		template <std::size_t I>
		static constexpr decltype(auto) get()
		{
			return lookup_table.template get_value<I>();
		}

		static constexpr decltype(auto) get(std::size_t index)
		{
			return lookup_table.get_value(index);
		}

		static constexpr decltype(auto) get(utility::string_units_utf8 name)
		{
			return get(find(name));
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
		static constexpr bool has(utility::string_units_utf8 name)
		{
			return lookup_table.contains(name);
		}

		static constexpr std::size_t find(utility::string_units_utf8 name)
		{
			return lookup_table.find(name);
		}

		template <typename X, typename F>
		static decltype(auto) call(std::size_t index, X && x, F && f)
		{
			switch (index)
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
			return detail::get_member(std::forward<X>(x), lookup_table.template get_value<I>());
		}

		template <typename X, typename F>
		static decltype(auto) call_with_all_members(X && x, F && f)
		{
			return call_with_all_members_impl(mpl::make_index_sequence<lookup_table.capacity>{}, std::forward<X>(x), std::forward<F>(f));
		}

		template <typename X, typename F>
		static void for_each_member(X && x, F && f)
		{
			for_each_member_impl(mpl::make_index_sequence<lookup_table.capacity>{}, std::forward<X>(x), std::forward<F>(f));
		}
	private:
#if defined(_MSC_VER)
# pragma warning( push )
# pragma warning( disable : 4702 )
		// C4702 - unreachable code
#endif
		template <typename X, typename F>
		static auto call_impl(mpl::index_constant<std::size_t(-1)>, X && x, F && f)
			-> decltype(f(detail::get_member(std::forward<X>(x), std::declval<lookup_table_t>().template get_value<0>())))
		{
			debug_unreachable();
			return f(detail::get_member(std::forward<X>(x), lookup_table.template get_value<0>()));
		}

		template <typename X, typename F>
		static auto call_impl(mpl::index_constant<std::size_t(-1)>, X && x, F && f)
			-> decltype(f(std::declval<lookup_table_t>().template get_key<0>(), detail::get_member(std::forward<X>(x), std::declval<lookup_table_t>().template get_value<0>())))
		{
			debug_unreachable();
			return f(lookup_table.template get_key<0>(), detail::get_member(std::forward<X>(x), lookup_table.template get_value<0>()));
		}
#if defined(_MSC_VER)
# pragma warning( pop )
#endif

		template <std::size_t I, typename X, typename F,
		          REQUIRES((I != std::size_t(-1)))>
		static auto call_impl(mpl::index_constant<I>, X && x, F && f)
			-> decltype(f(detail::get_member(std::forward<X>(x), std::declval<lookup_table_t>().template get_value<I>())))
		{
			return f(detail::get_member(std::forward<X>(x), lookup_table.template get_value<I>()));
		}

		template <std::size_t I, typename X, typename F,
		          REQUIRES((I != std::size_t(-1)))>
		static auto call_impl(mpl::index_constant<I>, X && x, F && f)
			-> decltype(f(std::declval<lookup_table_t>().template get_key<I>(), detail::get_member(std::forward<X>(x), std::declval<lookup_table_t>().template get_value<I>())))
		{
			return f(lookup_table.template get_key<I>(), detail::get_member(std::forward<X>(x), lookup_table.template get_value<I>()));
		}

		template <std::size_t ...Is, typename X, typename F>
		static decltype(auto) call_with_all_members_impl(mpl::index_sequence<Is...>, X && x, F && f)
		{
			return f(detail::get_member(std::forward<X>(x), lookup_table.template get_value<Is>())...);
		}

		template <std::size_t ...Is, typename X, typename F>
		static void for_each_member_impl(mpl::index_sequence<Is...>, X && x, F && f)
		{
			int expansion_hack[] = {(call_impl(mpl::index_constant<Is>{}, std::forward<X>(x), std::forward<F>(f)), 0)...};
			static_cast<void>(expansion_hack);
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

	template <typename T>
	auto clear(T & x)
		-> decltype(x.clear(), bool())
	{
		x.clear();
		return true;
	}

	namespace detail
	{
		template <typename T>
		bool clear(mpl::index_constant<std::size_t(-1)>, T &)
		{
			return false;
		}

		template <std::size_t I, typename T>
		bool clear(mpl::index_constant<I>, T & x)
		{
			return core::clear(core::member_table<T>::template get<I>(x));
		}
	}

	template <std::size_t I, typename T>
	bool clear(T & x)
	{
		return detail::clear(mpl::index_constant<I>{}, x);
	}

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

	namespace detail
	{
		template <typename T, typename F>
		bool for_each(mpl::index_constant<std::size_t(-1)>, T &, std::size_t, F &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename F>
		bool for_each(mpl::index_constant<I>, T & x, std::size_t count, F && f)
		{
			return core::for_each(core::member_table<T>::template get<I>(x), count, std::forward<F>(f));
		}
	}

	template <std::size_t I, typename T, typename F>
	bool for_each(T & x, std::size_t count, F && f)
	{
		return detail::for_each(mpl::index_constant<I>{}, x, count, std::forward<F>(f));
	}

	template <typename T>
	auto grow(T & x)
		-> decltype(x.emplace_back(), x.back())
	{
		x.emplace_back();
		return x.back();
	}

	namespace detail
	{
		// todo implement invoke and simplify object

		template <typename T, typename Object,
		          REQUIRES((core::has_lookup_table<T>::value))>
		auto serialize_enum(T & x, Object && object)
			-> decltype(core::value_table<T>::find(object()), bool())
		{
			const auto index = core::value_table<T>::find(object());
			if (index == std::size_t(-1))
				return debug_fail("unknown enum value");

			x = core::value_table<T>::get(index);
			return true;
		}

		template <typename T, typename Object,
		          REQUIRES((core::has_lookup_table<T>::value))>
		auto serialize_enum(T & x, Object && object)
			-> decltype(core::value_table<T>::find(std::forward<Object>(object)), bool())
		{
			const auto index = core::value_table<T>::find(std::forward<Object>(object));
			if (index == std::size_t(-1))
				return debug_fail("unknown enum value");

			x = core::value_table<T>::get(index);
			return true;
		}

		template <typename T, typename Object,
		          REQUIRES((std::is_enum<mpl::remove_cvref_t<T>>::value))>
		auto serialize(T & x, Object && object, int)
			-> decltype(serialize_enum(x, std::forward<Object>(object)))
		{
			return serialize_enum(x, std::forward<Object>(object));
		}

		template <typename T, typename Object>
		auto serialize_arithmetic(T & x, Object && object)
			-> decltype(x = object(), bool())
		{
			x = debug_cast<T>(object());
			return true;
		}

		template <typename T, typename Object>
		auto serialize_arithmetic(T & x, Object && object)
			-> decltype(x = std::forward<Object>(object), bool())
		{
			x = debug_cast<T>(std::forward<Object>(object));
			return true;
		}

		template <typename T, typename Value>
		auto serialize_arithmetic_impl(T & x, Value && value)
			-> decltype(ext::from_chars(value.data(), value.data() + value.size(), x), bool())
		{
			const auto result = ext::from_chars(value.data(), value.data() + value.size(), x);
			return result.ec == std::errc{};
		}

		template <typename T, typename Object>
		auto serialize_arithmetic(T & x, Object && object)
			-> decltype(serialize_arithmetic_impl(x, object()))
		{
			return serialize_arithmetic_impl(x, object());
		}

		template <typename T, typename Object>
		auto serialize_arithmetic(T & x, Object && object)
			-> decltype(serialize_arithmetic_impl(x, std::forward<Object>(object)))
		{
			return serialize_arithmetic_impl(x, std::forward<Object>(object));
		}

		template <typename T, typename Object,
		          REQUIRES((std::is_arithmetic<mpl::remove_cvref_t<T>>::value))>
		auto serialize(T & x, Object && object, int)
			-> decltype(serialize_arithmetic(x, std::forward<Object>(object)))
		{
			return serialize_arithmetic(x, std::forward<Object>(object));
		}

		template <typename T, typename Object>
		auto serialize(T & x, Object && object, float)
			-> decltype(x = object(), bool())
		{
			x = object();
			return true;
		}

		template <typename T, typename Object>
		auto serialize(T & x, Object && object, float)
			-> decltype(x = std::forward<Object>(object), bool())
		{
			x = std::forward<Object>(object);
			return true;
		}

		template <typename T, typename Object>
		bool serialize(T &, Object &&, ...)
		{
			constexpr auto value_name = utility::type_name<T>();
			constexpr auto object_name = utility::type_name<Object>();
			debug_unreachable("cannot serialize value of type '", value_name, "' to/from object of type '", object_name, "', maybe you are missing an overload to 'serialize'?");
		}
	}

	template <typename T, typename Object>
	bool serialize(T & x, Object && object)
	{
		return detail::serialize(x, std::forward<Object>(object), 0);
	}

	namespace detail
	{
		template <typename T, typename Object>
		bool serialize(mpl::index_constant<std::size_t(-1)>, T &, Object &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename Object>
		bool serialize(mpl::index_constant<I>, T & x, Object && object)
		{
			return core::serialize(core::member_table<T>::template get<I>(x), std::forward<Object>(object));
		}
	}

	template <std::size_t I, typename T, typename Object>
	bool serialize(T & x, Object && object)
	{
		return detail::serialize(mpl::index_constant<I>{}, x, std::forward<Object>(object));
	}

	template <typename T, typename Object>
	bool serialize(utility::optional<T> & x, Object && object)
	{
		return serialize(x ? x.value() : x.emplace(), std::forward<Object>(object));
	}
}
