#pragma once

#include "utility/concepts.hpp"
#include "utility/type_traits.hpp"

#include "ful/view.hpp"
#include "ful/string_search.hpp"

namespace core
{
	namespace file
	{
		inline ful::view_utf8 filename(ful::view_utf8 filepath)
		{
			auto sep = ful::rfind(filepath, ful::char8{'/'});
			if (sep == filepath.end())
			{
				sep = filepath.begin();
			}
			else
			{
				++sep; // do not include '/'
			}

			auto to = rfind(from(filepath, sep), ful::char8{'.'});

			return ful::view_utf8(sep, to);
		}

		// todo move to ful
		template <typename T>
		inline void replace(T * begin, T * end, T o, T n)
		{
			while (true)
			{
				const auto slash = ful::find(begin, end, o);
				if (slash == end)
					break;

				*slash = n;

				begin = slash + 1;
			}
		}

		template <typename T, REQUIRES(sizeof(T) == sizeof(ful::char8))>
		inline void backslash_to_slash(T * begin, T * end)
		{
			replace(reinterpret_cast<ful::char8 *>(begin), reinterpret_cast<ful::char8 *>(end), ful::char8{'\\'}, ful::char8{'/'});
		}

		template <typename T, REQUIRES(sizeof(T) == sizeof(ful::char16))>
		inline void backslash_to_slash(T * begin, T * end)
		{
			replace(reinterpret_cast<ful::char16 *>(begin), reinterpret_cast<ful::char16 *>(end), ful::char16{'\\'}, ful::char16{'/'});
		}

		template <typename R>
		inline void backslash_to_slash(R & x)
		{
			using ful::begin;
			using ful::end;

			backslash_to_slash(begin(x), end(x));
		}

		template <typename T, REQUIRES(sizeof(T) == sizeof(ful::char8))>
		inline void slash_to_backslash(T * begin, T * end)
		{
			replace(reinterpret_cast<ful::char8 *>(begin), reinterpret_cast<ful::char8 *>(end), ful::char8{'/'}, ful::char8{'\\'});
		}

		template <typename T, REQUIRES(sizeof(T) == sizeof(ful::char16))>
		inline void slash_to_backslash(T * begin, T * end)
		{
			replace(reinterpret_cast<ful::char16 *>(begin), reinterpret_cast<ful::char16 *>(end), ful::char16{'/'}, ful::char16{'\\'});
		}

		template <typename R>
		inline void slash_to_backslash(R & x)
		{
			using ful::begin;
			using ful::end;

			slash_to_backslash(begin(x), end(x));
		}
	}
}
