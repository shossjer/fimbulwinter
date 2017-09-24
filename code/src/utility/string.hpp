
#ifndef UTILITY_STRING_HPP
#define UTILITY_STRING_HPP

#include "stream.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace utility
{
	/**
	 */
	template <typename T>
	T &from_string(const std::string &string, T &t, const bool full_match = false)
	{
		std::istringstream stream(string);

		from_stream(stream, t);

		if (full_match && !stream.eof())
			throw std::invalid_argument("");

		return t;
	}
	/**
	 */
	template <typename T>
	inline T from_string(const std::string &string, const bool full_match = false)
	{
		T t;

		return from_string(string, t, full_match);
	}
	/**
	 */
	template <typename ...Ts>
	std::string to_string(Ts &&...ts)
	{
		std::ostringstream stream;

		to_stream(stream, std::forward<Ts>(ts)...);

		return stream.str();
	}

	/**
	 */
	template <std::size_t N>
	bool begins_with(const std::string &string, const char (&chars)[N])
	{
		if (string.size() < N) return false;

		for (unsigned int i = 0; i < N; ++i)
		{
			if (string[i] != chars[i]) return false;
		}
		return true;
	}

	/**
	 */
	inline std::vector<std::string> &split(const std::string &string, const char delimiter, std::vector<std::string> &words, const bool remove_whitespaces = false)
	{
		std::istringstream stream(string);

		return split(stream, delimiter, words, remove_whitespaces);
	}
	/**
	 */
	inline std::vector<std::string> split(const std::string &string, const char delimiter, const bool remove_whitespaces = false)
	{
		std::vector<std::string> words;

		return split(string, delimiter, words, remove_whitespaces);
	}
	
	/**
	 * Trim from start.
	 */
	inline std::string &trim_front(std::string &s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	/*
	 * Trim from end.
	 */
	inline std::string &trim_back(std::string &s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	/*
	 * Trim from both ends.
	 */
	inline std::string &trim(std::string &s)
	{
		return trim_front(trim_back(s));
	}

	/**
	 * Alias for `to_string`.
	 */
	template <typename ...Ts>
	inline std::string concat(Ts &&...ts)
	{
		return to_string(std::forward<Ts>(ts)...);
	}
}

#endif /* UTILITY_STRING_HPP */
