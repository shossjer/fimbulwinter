#pragma once

#include "utility/unicode.hpp"

namespace core
{
	namespace file
	{
		// todo generalize
		inline utility::string_view_utf8 filename(utility::string_view_utf8 filepath)
		{
			auto from = utility::unit_difference(filepath.rfind('/'));
			if (from == filepath.size())
			{
				from = utility::unit_difference(0);
			}
			else
			{
				from++; // do not include '/'
			}

			auto to = utility::unit_difference(filepath.rfind('.'));

			return utility::string_view_utf8(filepath, from, to - from);
		}

		template <typename StorageTraits, typename Encoding>
		utility::basic_string_view<Encoding> filename(const utility::basic_string<StorageTraits, Encoding> & filepath)
		{
			return filename(static_cast<utility::basic_string_view<Encoding>>(filepath));
		}
	}
}
