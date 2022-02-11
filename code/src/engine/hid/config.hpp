#pragma once

#include "core/serialization.hpp"

namespace engine
{
	namespace hid
	{
		struct config_t
		{
			bool hardware_input = true;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("hardware-input"), &config_t::hardware_input)
					);
			}
		};
	}
}
