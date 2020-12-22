#pragma once

#include "utility/unicode/string.hpp"

namespace core
{
	namespace file
	{
		// todo generalize
		inline utility::string_units_utf8 filename(utility::string_units_utf8 filepath)
		{
			auto from = rfind(filepath, '/');
			if (from == filepath.end())
			{
				from = filepath.begin();
			}
			else
			{
				++from; // do not include '/'
			}

			auto to = rfind(from, filepath.end(), '.');

			return utility::string_units_utf8(from, to);
		}

		template <typename StorageTraits, typename Encoding>
		utility::string_units_utf8 filename(const utility::basic_string<StorageTraits, Encoding> & filepath)
		{
			return filename(static_cast<utility::string_units_utf8>(filepath));
		}

		// todo make generic replace function
		template <typename BeginIt, typename EndIt>
		void backslash_to_slash(BeginIt begin, EndIt end)
		{
			using T = typename std::iterator_traits<BeginIt>::value_type;

			while (true)
			{
				const auto slash = find(begin, end, static_cast<T>('\\'));
				if (slash == end)
					break;

				*slash = static_cast<T>('/');

				begin = slash + 1;
			}
		}

		template <typename StorageTraits, typename Encoding>
		void backslash_to_slash(utility::basic_string<StorageTraits, Encoding> & filepath)
		{
			return backslash_to_slash(filepath.begin(), filepath.end());
		}

		template <typename BeginIt, typename EndIt>
		void slash_to_backslash(BeginIt begin, EndIt end)
		{
			using T = typename std::iterator_traits<BeginIt>::value_type;

			while (true)
			{
				const auto slash = find(begin, end, static_cast<T>('/'));
				if (slash == end)
					break;

				*slash = static_cast<T>('\\');

				begin = slash + 1;
			}
		}

		template <typename StorageTraits, typename Encoding>
		void slash_to_backslash(utility::basic_string<StorageTraits, Encoding> & filepath)
		{
			return slash_to_backslash(filepath.begin(), filepath.end());
		}
	}
}
