
#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include "core/serialization.hpp"

#include "engine/application/config.hpp"
#include "engine/graphics/config.hpp"
#include "engine/hid/config.hpp"
#include "engine/resource/formats.hpp"

struct settings_t
{
	struct general_t
	{
		engine::resource::Format settings_format = engine::resource::Format::Ini;

		static constexpr auto serialization()
		{
			return utility::make_lookup_table(
				std::make_pair(utility::string_view("settings_format"), &general_t::settings_format)
				);
		}
	};

	general_t general;
	engine::application::config_t application;
	engine::graphics::config_t graphics;
	engine::hid::config_t hid;

	static constexpr auto serialization()
	{
		return utility::make_lookup_table(
			std::make_pair(utility::string_view("general"), &settings_t::general),
			std::make_pair(utility::string_view("application"), &settings_t::application),
			std::make_pair(utility::string_view("graphics"), &settings_t::graphics),
			std::make_pair(utility::string_view("hid"), &settings_t::hid)
			);
	}
};

#endif /* SETTINGS_HPP */
