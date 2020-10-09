#pragma once

#include "core/serialization.hpp"

#include "engine/graphics/renderer.hpp"

namespace engine
{
	namespace graphics
	{
		struct config_t
		{
			renderer::Type renderer_type = renderer::Type::OPENGL_3_0;

			static constexpr auto serialization()
			{
				return utility::make_lookup_table(
					std::make_pair(utility::string_units_utf8("renderer"), &config_t::renderer_type)
					);
			}
		};
	}
}
