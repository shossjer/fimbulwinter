#pragma once

#include "core/serialization.hpp"

#include "engine/graphics/renderer.hpp"

namespace engine
{
	namespace graphics
	{
		struct config_t
		{
#if GRAPHICS_USE_OPENGL
			renderer::Type renderer_type = renderer::Type::OPENGL_3_0;
#else
			renderer::Type renderer_type = renderer::Type{};
#endif

			static constexpr auto serialization()
			{
				return utility::make_lookup_table<ful::view_utf8>(
					std::make_pair(ful::cstr_utf8("renderer"), &config_t::renderer_type)
					);
			}
		};
	}
}
