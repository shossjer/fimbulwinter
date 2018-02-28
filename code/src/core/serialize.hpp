
#ifndef CORE_SERIALIZE_HPP
#define CORE_SERIALIZE_HPP

#include "utility/type_traits.hpp"

#include <cstddef>
#include <iterator>
#include <tuple>

namespace core
{
	struct list_begin_t { explicit constexpr list_begin_t(int) {} };
	struct list_end_t { explicit constexpr list_end_t(int) {} };
	struct list_space_t { explicit constexpr list_space_t(int) {} };

	struct tuple_begin_t { explicit constexpr tuple_begin_t(int) {} };
	struct tuple_end_t { explicit constexpr tuple_end_t(int) {} };
	struct tuple_space_t { explicit constexpr tuple_space_t(int) {} };

	struct newline_t { explicit constexpr newline_t(int) {} };

	constexpr list_begin_t list_begin{ 0 };
	constexpr list_end_t list_end{ 0 };
	constexpr list_space_t list_space{ 0 };

	constexpr tuple_begin_t tuple_begin{ 0 };
	constexpr tuple_end_t tuple_end{ 0 };
	constexpr tuple_space_t tuple_space{ 0 };

	constexpr newline_t newline{ 0 };


	template <typename S>
	void serialize(S & s, std::nullptr_t x) { s(x); }

	template <typename S>
	void serialize(S & s, const bool & x) { s(x); }
	template <typename S>
	void serialize(S & s, bool & x) { s(x); }

	template <typename S>
	void serialize(S & s, const char & x) { s(x); }
	template <typename S>
	void serialize(S & s, char & x) { s(x); }
	template <typename S>
	void serialize(S & s, const char16_t & x) { s(x); }
	template <typename S>
	void serialize(S & s, char16_t & x) { s(x); }
	template <typename S>
	void serialize(S & s, const char32_t & x) { s(x); }
	template <typename S>
	void serialize(S & s, char32_t & x) { s(x); }
	template <typename S>
	void serialize(S & s, const wchar_t & x) { s(x); }
	template <typename S>
	void serialize(S & s, wchar_t & x) { s(x); }

	template <typename S>
	void serialize(S & s, const signed char & x) { s(x); }
	template <typename S>
	void serialize(S & s, signed char & x) { s(x); }
	template <typename S>
	void serialize(S & s, const unsigned char & x) { s(x); }
	template <typename S>
	void serialize(S & s, unsigned char & x) { s(x); }
	template <typename S>
	void serialize(S & s, const short int & x) { s(x); }
	template <typename S>
	void serialize(S & s, short int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const unsigned short int & x) { s(x); }
	template <typename S>
	void serialize(S & s, unsigned short int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const int & x) { s(x); }
	template <typename S>
	void serialize(S & s, int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const unsigned int & x) { s(x); }
	template <typename S>
	void serialize(S & s, unsigned int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const unsigned long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, unsigned long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const long long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, long long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, const unsigned long long int & x) { s(x); }
	template <typename S>
	void serialize(S & s, unsigned long long int & x) { s(x); }

	template <typename S>
	void serialize(S & s, const float & x) { s(x); }
	template <typename S>
	void serialize(S & s, float & x) { s(x); }
	template <typename S>
	void serialize(S & s, const double & x) { s(x); }
	template <typename S>
	void serialize(S & s, double & x) { s(x); }
	template <typename S>
	void serialize(S & s, const long double & x) { s(x); }
	template <typename S>
	void serialize(S & s, long double & x) { s(x); }

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
		template <typename S, typename ...Ts, size_t ...Is>
		void serialize_tuple(S & s, std::tuple<Ts...> & x, mpl::index_sequence<Is...>)
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
	template <typename S, typename ...Ts>
	void serialize(S & s, std::tuple<Ts...> & x)
	{
		s(tuple_begin);

		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});

		s(tuple_end);
	}

	template <typename S, typename T>
	void serialize_impl(S & s, T & t, ...)
	{
		s(list_begin);
		serialize_class(s, t);
		s(list_end);
	}

	template <typename S, typename T>
	auto serialize_impl(S & s, T & xs, int) ->
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

	template <typename S, typename T>
	void serialize(S & s, T & t)
	{
		serialize_impl(s, t, 0);
	}
}

#endif /* CORE_SERIALIZE_HPP */
