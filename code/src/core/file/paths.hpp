#pragma once

#include "utility/unicode/string.hpp"

namespace core
{
	namespace file
	{
		// todo generalize
		inline utility::string_units_utf8 filename(utility::string_units_utf8 filepath)
		{
			auto from = filepath.rfind('/');
			if (static_cast<ext::usize>(from) == filepath.size())
			{
				from = 0;
			}
			else
			{
				from++; // do not include '/'
			}

			auto to = filepath.rfind('.');

			return utility::string_units_utf8(filepath, from, to - from);
		}

		template <typename StorageTraits, typename Encoding>
		utility::string_units_utf8 filename(const utility::basic_string<StorageTraits, Encoding> & filepath)
		{
			return filename(static_cast<utility::string_units_utf8>(filepath));
		}
	}
}
