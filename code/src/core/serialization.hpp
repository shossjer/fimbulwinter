#pragma once

#include "core/debug.hpp"

#include "utility/charconv.hpp"
#include "utility/iterator.hpp"
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
		template <typename T>
		struct array_element_t
		{
			T member_;
			std::size_t index_;

			constexpr explicit array_element_t(T member, std::size_t index)
				: member_(member)
				, index_(index)
			{}

			template <typename X>
			decltype(auto) get(X && x) const
			{
				return (x.*member_)[index_];
			}

		};
	}

	template <typename T, typename C>
	constexpr auto array_element(T C:: * member, std::size_t index)
	{
		return detail::array_element_t<T C:: *>(member, index);
	}

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
			-> decltype(f(std::declval<lookup_table_t>().template get_key<I>(), detail::get_member(std::forward<X>(x), std::declval<lookup_table_t>().template get_value<0>())))
		{
			debug_unreachable();
			return f(lookup_table.template get_key<I>(), detail::get_member(std::forward<X>(x), lookup_table.template get_value<0>()));
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
		bool clear_impl(mpl::index_constant<std::size_t(-1)>, T &)
		{
			return false;
		}

		template <std::size_t I, typename T>
		auto clear_impl(mpl::index_constant<I>, T & x)
			-> decltype(clear(core::member_table<T>::template get<I>(x)))
		{
			return clear(core::member_table<T>::template get<I>(x));
		}
	}

	template <std::size_t I, typename T>
	auto clear(T & x)
		-> decltype(detail::clear_impl(mpl::index_constant<I>{}, x))
	{
		return detail::clear_impl(mpl::index_constant<I>{}, x);
	}

	namespace detail
	{
		namespace copy_impl_range_space
		{
			using utility::begin;

			template <typename T, typename BeginIt, typename EndIt>
			auto copy_impl(T & x, BeginIt ibegin, EndIt iend, int)
				-> decltype(*begin(x) = *ibegin, bool())
			{
				debug_expression(using utility::end);

				const auto obegin = begin(x);
				debug_expression(const auto oend = end(x));
				if (!debug_assert(iend - ibegin == oend - obegin))
					return false;

				std::copy(ibegin, iend, obegin);

				return true;
			}
		}
		using copy_impl_range_space::copy_impl;

		namespace copy_impl_tuple_space
		{
			using ext::get;

			template <typename Tuple, typename BeginIt, typename EndIt, std::size_t ...Is>
			auto copy_impl_tuple(Tuple & tuple, BeginIt ibegin, EndIt iend, mpl::index_sequence<Is...>)
				-> decltype(ext::declexpand(get<Is>(tuple) = *ibegin...), bool())
			{
				int expansion_hack[] = {(get<Is>(tuple) = *ibegin, ++ibegin, 0)...};
				static_cast<void>(expansion_hack);

				return true;
			}
		}
		using copy_impl_tuple_space::copy_impl_tuple;

		template <typename Tuple, typename BeginIt, typename EndIt,
		          REQUIRES((ext::is_tuple<Tuple &>::value))>
		auto copy_impl(Tuple & tuple, BeginIt ibegin, EndIt iend, ...)
			-> decltype(copy_impl_tuple(tuple, ibegin, iend, ext::make_tuple_sequence<Tuple>{}))
		{
			return copy_impl_tuple(tuple, ibegin, iend, ext::make_tuple_sequence<Tuple>{});
		}
	}

	template <typename T, typename BeginIt, typename EndIt>
	auto copy(T & x, BeginIt ibegin, EndIt iend)
		-> decltype(detail::copy_impl(x, ibegin, iend, 0))
	{
		return detail::copy_impl(x, ibegin, iend, 0);
	}

	template <typename T, typename BeginIt, typename EndIt>
	using supports_copy = decltype(copy(std::declval<T>(), std::declval<BeginIt>(), std::declval<EndIt>()));

	namespace detail
	{
		template <typename T, typename BeginIt, typename EndIt>
		bool copy_impl(mpl::index_constant<std::size_t(-1)>, T &, BeginIt, EndIt)
		{
			return false;
		}

		template <std::size_t I, typename T, typename BeginIt, typename EndIt>
		auto copy_impl(mpl::index_constant<I>, T & x, BeginIt ibegin, EndIt iend)
			-> decltype(copy(core::member_table<T>::template get<I>(x), ibegin, iend))
		{
			return copy(core::member_table<T>::template get<I>(x), ibegin, iend);
		}
	}

	template <std::size_t I, typename T, typename BeginIt, typename EndIt>
	auto copy(T & x, BeginIt ibegin, EndIt iend)
		-> decltype(detail::copy_impl(mpl::index_constant<I>{}, x, ibegin, iend))
	{
		return detail::copy_impl(mpl::index_constant<I>{}, x, ibegin, iend);
	}

	namespace detail
	{
		template <typename T, typename F,
		          REQUIRES((utility::is_range<T &>::value))>
		bool for_each_impl(T & x, F && f, int)
		{
			for (auto && y : x)
			{
				if (!f(y))
					return false;
			}
			return true;
		}

		template <typename F>
		bool for_each_impl_tuple(F &&)
		{
			return true;
		}

		template <typename F, typename T, typename ...Ts>
		bool for_each_impl_tuple(F && f, T & x, Ts & ...xs)
		{
			if (!f(x))
				return false;

			return for_each_impl_tuple(std::forward<F>(f), xs...);
		}

		template <typename T, typename F,
		          REQUIRES((ext::is_tuple<T &>::value))>
		bool for_each_impl(T & x, F && f, ...)
		{
			ext::apply([&](auto & ...ys){ return for_each_impl_tuple(std::forward<F>(f), ys...); }, x);
		}
	}

	template <typename T, typename F>
	auto for_each(T & x, F && f)
		-> decltype(detail::for_each_impl(x, std::forward<F>(f), 0))
	{
		return detail::for_each_impl(x, std::forward<F>(f), 0);
	}

	template <typename T, typename F>
	using supports_for_each = decltype(for_each(std::declval<T>(), std::declval<F>()));

	namespace detail
	{
		template <typename T, typename F>
		bool for_each_impl(mpl::index_constant<std::size_t(-1)>, T &, std::size_t, F &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename F>
		auto for_each_impl(mpl::index_constant<I>, T & x, std::size_t count, F && f)
			-> decltype(for_each(core::member_table<T>::template get<I>(x), count, std::forward<F>(f)))
		{
			return for_each(core::member_table<T>::template get<I>(x), count, std::forward<F>(f));
		}
	}

	template <std::size_t I, typename T, typename F>
	auto for_each(T & x, std::size_t count, F && f)
		-> decltype(detail::for_each_impl(mpl::index_constant<I>{}, x, count, std::forward<F>(f)))
	{
		return detail::for_each_impl(mpl::index_constant<I>{}, x, count, std::forward<F>(f));
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
		template <typename T>
		auto resize_impl(T & x, std::size_t count, int)
			-> decltype(x.resize(count), bool())
		{
			x.resize(count);

			return true;
		}

		namespace resize_impl_size_space
		{
			using ext::size;

			template <typename T>
			auto resize_impl(T & x, std::size_t count, ...)
				-> decltype(size(x), bool())
			{
				return size(x) == count;
			}
		}
		using resize_impl_size_space::resize_impl;
	}

	template <typename T>
	auto resize(T & x, std::size_t count)
		-> decltype(detail::resize_impl(x, count, 0))
	{
		return detail::resize_impl(x, count, 0);
	}

	template <typename T>
	using supports_resize = decltype(resize(std::declval<T>(), 0));

	template <typename U, typename T>
	auto reshape(T & x, std::size_t count)
		-> decltype(x.template reshape<U>(count))
	{
		return x.template reshape<U>(count);
	}

	template <typename U, typename T>
	using supports_reshape = decltype(reshape<U>(std::declval<T>(), 0));

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
		auto serialize_impl(T & x, Object && object, int)
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
		auto serialize_impl(T & x, Object && object, int)
			-> decltype(serialize_arithmetic(x, std::forward<Object>(object)))
		{
			return serialize_arithmetic(x, std::forward<Object>(object));
		}

		template <typename T, typename Object>
		auto serialize_impl(T & x, Object && object, float)
			-> decltype(x = object(), bool())
		{
			x = object();
			return true;
		}

		template <typename T, typename Object>
		auto serialize_impl(T & x, Object && object, float)
			-> decltype(x = std::forward<Object>(object), bool())
		{
			x = std::forward<Object>(object);
			return true;
		}

		template <typename T, typename Object>
		bool serialize_impl(T &, Object &&, ...)
		{
			constexpr auto value_name = utility::type_name<T>();
			constexpr auto object_name = utility::type_name<Object>();
			debug_unreachable("cannot serialize value of type '", value_name, "' to/from object of type '", object_name, "', maybe you are missing an overload to 'serialize'?");
		}
	}

	template <typename T, typename Object>
	bool serialize(T & x, Object && object)
	{
		return detail::serialize_impl(x, std::forward<Object>(object), 0);
	}

	template <typename T, typename Object>
	bool serialize(utility::optional<T> & x, Object && object)
	{
		return serialize(x ? x.value() : x.emplace(), std::forward<Object>(object));
	}

	namespace detail
	{
		template <typename T, typename Object>
		bool serialize_impl(mpl::index_constant<std::size_t(-1)>, T &, Object &&)
		{
			return false;
		}

		template <std::size_t I, typename T, typename Object>
		auto serialize_impl(mpl::index_constant<I>, T & x, Object && object)
			-> decltype(serialize(core::member_table<T>::template get<I>(x), std::forward<Object>(object)))
		{
			return serialize(core::member_table<T>::template get<I>(x), std::forward<Object>(object));
		}
	}

	template <std::size_t I, typename T, typename Object>
	auto serialize(T & x, Object && object)
		-> decltype(detail::serialize_impl(mpl::index_constant<I>{}, x, std::forward<Object>(object)))
	{
		return detail::serialize_impl(mpl::index_constant<I>{}, x, std::forward<Object>(object));
	}
}
