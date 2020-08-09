#pragma once

#include "utility/ext/stddef.hpp"

#include <iterator>

namespace ext
{
	template <typename It, typename Char = typename std::iterator_traits<It>::value_type>
	constexpr It strend(It str, Char term = Char{})
	{
		for (; *str != term; ++str);
		return str;
	}

	template <typename It, typename Char = typename std::iterator_traits<It>::value_type>
	constexpr usize strlen(It str, Char term = Char{})
	{
		return strend(str, term) - str;
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

	template <typename BeginIt1, typename EndIt1, typename BeginIt2>
	constexpr int strlcmp(BeginIt1 begin1, EndIt1 end1, BeginIt2 begin2)
	{
		for (; begin1 != end1; ++begin1, ++begin2)
		{
			if (*begin1 != *begin2)
				return *begin1 < *begin2 ? -1 : 1;
		}
		return 0;
	}

	template <typename It1, typename BeginIt2, typename EndIt2>
	constexpr It1 strfind(It1 begin, It1 end, BeginIt2 exprbegin, EndIt2 exprend)
	{
		if (end - begin < exprend - exprbegin)
			return end;

		end -= exprend - exprbegin;

		for (; !(strlcmp(exprbegin, exprend, begin) == 0 || begin == end); ++begin);
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

		for (; strlcmp(exprbegin, exprend, cur) != 0; --cur)
		{
			if (cur == begin)
				return end;
		}

		return cur;
	}
}
