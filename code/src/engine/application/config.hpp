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
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("window_width"), &config_t::window_width),
					std::make_pair(ful::cstr_utf8("window_height"), &config_t::window_height)
					);
			}
		};
	}
}
