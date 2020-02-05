#pragma once

#include "utility/ext/stddef.hpp"

namespace ext
{
	template <typename Char>
	constexpr usize strlen(const Char * str, Char term = Char{})
	{
		const Char * const begin = str;
		for (; *str != term; str++);
		return static_cast<usize>(str - begin);
	}

	template <typename Char>
	constexpr const Char * strfind(const Char * begin, const Char * end, Char c)
	{
		for (; begin != end; begin++)
		{
			if (*begin == c)
				break;
		}

		return begin;
	}

	template <typename Char>
	constexpr const Char * strrfind(const Char * begin, const Char * end, Char c)
	{
		const Char * cur = end;
		for (; cur != begin;)
		{
			cur--;
			if (*cur == c)
				return cur;
		}

		return end;
	}
}
