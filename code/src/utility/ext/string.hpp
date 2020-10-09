#pragma once

#include "utility/ext/stddef.hpp"

#include <iterator>

namespace ext
{
	template <typename It>
	constexpr It strend(It str, typename std::iterator_traits<It>::value_type term = typename std::iterator_traits<It>::value_type{})
	{
		for (; *str != term; ++str);
		return str;
	}

	template <typename It>
	constexpr usize strlen(It str, typename std::iterator_traits<It>::value_type term = typename std::iterator_traits<It>::value_type{})
	{
		return strend(str, term) - str;
	}

	template <typename BeginIt1, typename EndIt1, typename BeginIt2>
	constexpr bool strmatch(BeginIt1 begin1, EndIt1 end1, BeginIt2 begin2)
	{
		for (; begin1 != end1; ++begin1, ++begin2)
		{
			if (*begin1 != *begin2)
				return false;
		}
		return true;
	}

	template <typename BeginIt, typename EndIt>
	constexpr bool strbegins(BeginIt begin, EndIt end, typename std::iterator_traits<BeginIt>::value_type c)
	{
		return begin == end ? false : *begin == c;
	}

	template <typename BeginIt, typename EndIt>
	constexpr bool strbegins(BeginIt begin1, EndIt end1, BeginIt begin2)
	{
		for (; begin1 != end1; ++begin1, ++begin2)
		{
			if (*begin1 != *begin2)
				break;
		}
		return *begin2 == typename std::iterator_traits<BeginIt>::value_type{};
	}

	template <typename BeginIt1, typename EndIt1, typename BeginIt2, typename EndIt2>
	constexpr bool strbegins(BeginIt1 begin1, EndIt1 end1, BeginIt2 begin2, EndIt2 end2)
	{
		const auto size1 = end1 - begin1;
		const auto size2 = end2 - begin2;
		if (size1 < size2)
			return false;

		return strmatch(begin2, end2, begin1);
	}

	template <typename BeginIt1, typename BeginIt2>
	constexpr int strncmp(BeginIt1 begin1, BeginIt2 begin2, usize size)
	{
		const auto end1 = begin1 + size;

		for (; begin1 != end1; ++begin1, ++begin2)
		{
			if (*begin1 != *begin2)
				return *begin1 < *begin2 ? -1 : 1;
		}
		return 0;
	}

	template <typename BeginIt, typename EndIt>
	constexpr int strcmp(BeginIt begin, EndIt end, typename std::iterator_traits<BeginIt>::value_type c)
	{
		if (begin == end)
			return -1;

		if (*begin != c)
			return *begin < c ? -1 : 1;

		++begin;

		return begin == end ? 0 : 1;
	}

	template <typename BeginIt, typename EndIt>
	constexpr int strcmp(BeginIt begin1, EndIt end1, BeginIt begin2)
	{
		for (; begin1 != end1; ++begin1, ++begin2)
		{
			if (*begin1 != *begin2)
				return *begin1 < *begin2 ? -1 : 1;
		}
		return *begin2 == typename std::iterator_traits<BeginIt>::value_type{} ? 0 : -1;
	}

	template <typename It1, typename It2>
	constexpr int strcmp(It1 begin1, It1 end1, It2 begin2, It2 end2)
	{
		const auto size1 = end1 - begin1;
		const auto size2 = end2 - begin2;

		const int result = strncmp(begin1, begin2, size2 < size1 ? size2 : size1);

		return
			result != 0 ? result :
			size1 < size2 ? -1 :
			size2 < size1 ? 1 :
			0;
	}

	template <typename BeginIt, typename EndIt, typename Char>
	constexpr BeginIt strfind(BeginIt begin, EndIt end, Char c)
	{
		for (; begin != end; ++begin)
		{
			if (*begin == c)
				break;
		}

		return begin;
	}

	template <typename It1, typename BeginIt2, typename EndIt2>
	constexpr It1 strfind(It1 begin, It1 end, BeginIt2 exprbegin, EndIt2 exprend)
	{
		if (end - begin < exprend - exprbegin)
			return end;

		const auto last = end - (exprend - exprbegin);

		for (; !strmatch(exprbegin, exprend, begin); ++begin)
		{
			if (begin == last)
				return end;
		}

		return begin;
	}

	template <typename It, typename Char>
	constexpr It strrfind(It begin, It end, Char c)
	{
		It cur = end;
		for (; cur != begin;)
		{
			--cur;
			if (*cur == c)
				return cur;
		}

		return end;
	}

	template <typename It1, typename BeginIt2, typename EndIt2>
	constexpr It1 strrfind(It1 begin, It1 end, BeginIt2 exprbegin, EndIt2 exprend)
	{
		if (end - begin < exprend - exprbegin)
			return end;

		auto cur = end - (exprend - exprbegin);

		for (; !strmatch(exprbegin, exprend, cur); --cur)
		{
			if (cur == begin)
				return end;
		}

		return cur;
	}
}
