
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_RESOURCES_HPP
#define ENGINE_GUI_RESOURCES_HPP

#include <engine/Asset.hpp>

#include <cstdint>

namespace engine
{
	namespace gui
	{
		class View;

		namespace resource
		{
			using ColorValue = uint32_t;

			struct Color
			{
				virtual ColorValue get(const View * view) const = 0;
			};

			const Color * color(const engine::Asset asset);

			void put(const engine::Asset asset, const ColorValue val);

			void put(const engine::Asset asset, const engine::Asset def, const engine::Asset high, const engine::Asset press);

			void purge();
		}
	}
}

#endif
