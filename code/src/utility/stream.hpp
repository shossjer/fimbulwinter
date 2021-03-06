#pragma once

#include "type_info.hpp"
#include "type_traits.hpp"

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace mpl
{
	template <typename T, typename = void>
	struct is_ostreamable_impl : mpl::false_type {};
	template <typename T>
	struct is_ostreamable_impl<T, mpl::void_t<decltype(std::declval<std::ostream &>() << std::declval<T>())>> : mpl::true_type {};
	template <typename T>
	using is_ostreamable = is_ostreamable_impl<T>;
}

namespace utility
{
	inline bool from_stream(std::istream & /*stream*/)
	{
		return true;
	}

	template <typename T, typename ...Ts>
	inline bool from_stream(std::istream & stream, T & t, Ts & ...ts)
	{
		if (stream >> t)
			return from_stream(stream, ts...);
		else
			return false;
	}

	inline std::ostream &to_stream(std::ostream &stream)
	{
		return stream;
	}
	/**
	 */
	template <typename T, typename ...Ts>
	inline std::ostream &to_stream(std::ostream &stream, T &&t, Ts &&...ts)
	{
		if (stream << std::forward<T>(t)) return to_stream(stream, std::forward<Ts>(ts)...);

		throw std::invalid_argument("");
	}

	/**
	 */
	template <typename T>
	struct try_stream_yes_t
	{
		T && t;

		try_stream_yes_t(T && t) : t(t) {}

		friend std::ostream & operator << (std::ostream & stream, try_stream_yes_t<T> && t)
		{
			return stream << std::forward<T>(t.t);
		}
	};
	template <typename T>
	struct try_stream_no_t
	{
		friend std::ostream & operator << (std::ostream & stream, try_stream_no_t<T> &&)
		{
			constexpr auto signature = utility::type_signature<mpl::remove_cvref_t<T>>();
			return stream << "?(" << signature << ")";
		}
	};
	template <typename T,
	          mpl::enable_if_t<mpl::is_ostreamable<T>::value, int> = 0>
	try_stream_yes_t<T> try_stream(T && t)
	{
		return try_stream_yes_t<T>{t};
	}
	template <typename T,
	          mpl::disable_if_t<mpl::is_ostreamable<T>::value, int> = 0>
	try_stream_no_t<T> try_stream(T &&)
	{
		return try_stream_no_t<T>{};
	}

	/**
	 */
	inline std::vector<std::string> &split(std::istream &stream, const char delimiter, std::vector<std::string> &words, const bool /*remove_whitespaces*/ = false)
	{
		words.emplace_back();
		do
		{
			if (!words.back().empty())
				words.emplace_back();
		}
		while (std::getline(stream, words.back(), delimiter));

		words.pop_back();

		return words;
	}
}
