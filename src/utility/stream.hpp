
#ifndef UTILITY_STREAM_HPP
#define UTILITY_STREAM_HPP

#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace utility
{
	/**
	 */
	inline std::istream &from_stream(std::istream &stream)
	{
		return stream;
	}
	/**
	 */
	template <typename T, typename ...Ts>
	inline std::istream &from_stream(std::istream &stream, T &t, Ts &...ts)
	{
		if (stream >> t) return from_stream(stream, ts...);

		throw std::invalid_argument("");
	}
	/**
	 */
	template <typename T>
	inline T from_stream(std::istream &stream)
	{
		T t;
		{
			from_stream(stream, t);
		}
		return t;
	}
	/**
	 */
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
	inline std::vector<std::string> &split(std::istream &stream, const char delimiter, std::vector<std::string> &words, const bool remove_whitespaces = false)
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

#endif /* UTILITY_STREAM_HPP */
