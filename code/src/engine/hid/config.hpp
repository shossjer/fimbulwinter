
#ifndef ENGINE_HID_CONFIG_HPP
#define ENGINE_HID_CONFIG_HPP

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
					std::make_pair(utility::string_view("hardware-input"), &config_t::hardware_input)
					);
			}
		};
	}
}

#endif /* ENGINE_HID_CONFIG_HPP */
