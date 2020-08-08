#pragma once

#include "core/serialization.hpp"

namespace engine
{
	namespace application
	{
		struct config_t
		{
			int window_width = 720;
			int window_height = 405;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("window_width"), &config_t::window_width),
					std::make_pair(utility::string_units_utf8("window_height"), &config_t::window_height)
					);
			}
		};
	}
}
