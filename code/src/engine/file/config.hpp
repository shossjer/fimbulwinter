#pragma once

#include "core/serialization.hpp"

namespace engine
{
	namespace file
	{
		struct config_t
		{
			ext::usize write_size = static_cast<ext::usize>(1) << 26;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("write_size"), &config_t::write_size)
					);
			}
		};
	}
}
