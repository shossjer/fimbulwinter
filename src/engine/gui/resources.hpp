
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_RESOURCES_HPP
#define ENGINE_GUI_RESOURCES_HPP

#include <engine/Asset.hpp>
#include <engine/graphics/renderer.hpp>

#include <core/container/Collection.hpp>

namespace engine
{
namespace gui
{
	using Color = engine::graphics::data::Color;

	class View;

	namespace resource
	{

		struct ColorResource
		{
			virtual Color get(const View * view) const = 0;
		};

		const ColorResource * color(const engine::Asset asset);

		void put(const engine::Asset asset, const Color val);

		void put(const engine::Asset asset, const engine::Asset def, const engine::Asset high, const engine::Asset press);
	}
}
}

#endif // ENGINE_GUI_RESOURCES_HPP
