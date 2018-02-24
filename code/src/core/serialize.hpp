
#ifndef CORE_SERIALIZE_HPP
#define CORE_SERIALIZE_HPP

#include "utility/type_traits.hpp"

#include <cstddef>
#include <iterator>
#include <tuple>

namespace core
{
	struct list_begin_t { explicit constexpr list_begin_t(int) {} } constexpr list_begin(0);
	struct list_end_t { explicit constexpr list_end_t(int) {} } constexpr list_end(0);
	struct list_space_t { explicit constexpr list_space_t(int) {} } constexpr list_space(0);

	struct tuple_begin_t { explicit constexpr tuple_begin_t(int) {} } constexpr tuple_begin(0);
	struct tuple_end_t { explicit constexpr tuple_end_t(int) {} } constexpr tuple_end(0);
	struct tuple_space_t { explicit constexpr tuple_space_t(int) {} } constexpr tuple_space(0);

	struct newline_t { explicit constexpr newline_t(int) {} } constexpr newline(0);

	template <typename S>
	void serialize(S & s, std::nullptr_t x)
	{
		s(x);
	}

	template <typename S>
	void serialize(S & s, bool x)
	{
		s(x);
	}

	template <typename S>
	void serialize(S & s, char x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, char16_t x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, char32_t x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, wchar_t x)
	{
		s(x);
	}

	template <typename S>
	void serialize(S & s, signed char x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, unsigned char x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, short int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, unsigned short int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, unsigned int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, long int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, unsigned long int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, long long int x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, unsigned long long int x)
	{
		s(x);
	}

	template <typename S>
	void serialize(S & s, float x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, double x)
	{
		s(x);
	}
	template <typename S>
	void serialize(S & s, long double x)
	{
		s(x);
	}

	template <typename S, typename T>
	auto serialize(S & s, const T & xs) ->
		decltype(std::begin(xs), std::end(xs), void())
	{
		s(list_begin);

		bool not_first = false;
		for (auto && x : xs)
		{
			if (not_first)
			{
				s(list_space);
			}
			not_first = true;

			serialize(s, x);
		}

		s(list_end);
	}

	namespace detail
	{
		template <typename S>
		void serialize_tuple_impl(S & s)
		{
		}
		template <typename S, typename P1>
		void serialize_tuple_impl(S & s, P1 && p1)
		{
			serialize(s, std::forward<P1>(p1));
		}
		template <typename S, typename P1, typename P2, typename ...Ps>
		void serialize_tuple_impl(S & s, P1 && p1, P2 && p2, Ps && ...ps)
		{
			serialize(s, std::forward<P1>(p1));
			s(tuple_space);

			serialize_tuple_impl(s, std::forward<P2>(p2), std::forward<Ps>(ps)...);
		}

		template <typename S, typename ...Ts, size_t ...Is>
		void serialize_tuple(S & s, const std::tuple<Ts...> & x, mpl::index_sequence<Is...>)
		{
			serialize_tuple_impl(s, std::get<Is>(x)...);
		}
	}

	template <typename S, typename ...Ts>
	void serialize(S & s, const std::tuple<Ts...> & x)
	{
		s(tuple_begin);

		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});

		s(tuple_end);
	}
}

#endif /* CORE_SERIALIZE_HPP */
