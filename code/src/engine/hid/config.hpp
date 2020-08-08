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
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("hardware-input"), &config_t::hardware_input)
					);
			}
		};
	}
}
