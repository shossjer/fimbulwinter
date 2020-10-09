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
	}
}
