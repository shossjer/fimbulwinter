
#ifndef ENGINE_APPLICATION_CONFIG_HPP
#define ENGINE_APPLICATION_CONFIG_HPP

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
					std::make_pair(utility::string_view("window_width"), &config_t::window_width),
					std::make_pair(utility::string_view("window_height"), &config_t::window_height)
					);
			}
		};
	}
}

#endif /* ENGINE_APPLICATION_CONFIG_HPP */
