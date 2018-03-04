
#ifndef CORE_SERIALIZE_HPP
#define CORE_SERIALIZE_HPP

#include "utility/type_traits.hpp"

#include <cstddef>
#include <iterator>
#include <tuple>

namespace core
{
	struct type_class_t { explicit constexpr type_class_t(int) {} };
	struct type_list_t { explicit constexpr type_list_t(int) {} };
	struct type_tuple_t { explicit constexpr type_tuple_t(int) {} };

	constexpr type_class_t type_class{ 0 };
	constexpr type_list_t type_list{ 0 };
	constexpr type_tuple_t type_tuple{ 0 };


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

	template <typename S>
	void serialize(S & s, const std::string & x) { s(x); }
	template <typename S>
	void serialize(S & s, std::string & x) { s(x); }

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
		s.push(type_tuple, x);
		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});
		s.pop();
	}
	template <typename S, typename ...Ts>
	void serialize(S & s, std::tuple<Ts...> & x)
	{
		s.push(type_tuple, x);
		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});
		s.pop();
	}

	template <typename S, typename T>
	void serialize_impl(S & s, T & x, ...)
	{
		s.push(type_class, x);
		serialize_class(s, x);
		s.pop();
	}

	template <typename S, typename T>
	auto serialize_impl(S & s, T & xs, int) ->
		decltype(std::begin(xs), std::end(xs), void())
	{
		s.push(type_list, xs);
		for (auto && x : xs)
		{
			serialize(s, x);
		}
		s.pop();
	}

	template <typename S, typename T>
	void serialize(S & s, T & x)
	{
		serialize_impl(s, x, 0);
	}


	template <typename S>
	void serialize(S & s, const char * key, std::nullptr_t x) { s(key, x); }

	template <typename S>
	void serialize(S & s, const char * key, const bool & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, bool & x) { s(key, x); }

	template <typename S>
	void serialize(S & s, const char * key, const char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const char16_t & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, char16_t & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const char32_t & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, char32_t & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const wchar_t & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, wchar_t & x) { s(key, x); }

	template <typename S>
	void serialize(S & s, const char * key, const signed char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, signed char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const unsigned char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, unsigned char & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const short int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, short int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const unsigned short int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, unsigned short int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const unsigned int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, unsigned int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const unsigned long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, unsigned long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const long long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, long long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const unsigned long long int & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, unsigned long long int & x) { s(key, x); }

	template <typename S>
	void serialize(S & s, const char * key, const float & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, float & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const double & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, double & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, const long double & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, long double & x) { s(key, x); }

	template <typename S>
	void serialize(S & s, const char * key, const std::string & x) { s(key, x); }
	template <typename S>
	void serialize(S & s, const char * key, std::string & x) { s(key, x); }

	template <typename S, typename ...Ts>
	void serialize(S & s, const char * key, const std::tuple<Ts...> & x)
	{
		s.push(type_tuple, key, x);
		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});
		s.pop();
	}
	template <typename S, typename ...Ts>
	void serialize(S & s, const char * key, std::tuple<Ts...> & x)
	{
		s.push(type_tuple, key, x);
		detail::serialize_tuple(s, x, mpl::make_index_sequence_for<Ts...>{});
		s.pop();
	}

	template <typename S, typename T>
	void serialize_impl(S & s, const char * key, T & x, ...)
	{
		s.push(type_class, key, x);
		serialize_class(s, x);
		s.pop();
	}

	template <typename S, typename T>
	auto serialize_impl(S & s, const char * key, T & xs, int) ->
		decltype(std::begin(xs), std::end(xs), void())
	{
		s.push(type_list, key, xs);
		for (auto && x : xs)
		{
			serialize(s, x);
		}
		s.pop();
	}

	template <typename S, typename T>
	void serialize(S & s, const char * key, T & x)
	{
		serialize_impl(s, key, x, 0);
	}
}

#endif /* CORE_SERIALIZE_HPP */
