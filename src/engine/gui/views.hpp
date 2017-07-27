
#ifndef ENGINE_GUI_VIEWS_HPP
#define ENGINE_GUI_VIEWS_HPP

#include <engine/Asset.hpp>
#include <engine/common.hpp>
#include <engine/Entity.hpp>
#include <engine/graphics/renderer.hpp>

#include <vector>

namespace engine
{
namespace gui
{
	struct Data
	{
		engine::graphics::data::Color color;
		std::string display;
		engine::Asset texture;
		std::vector<std::pair<engine::Asset, Data>> list;
		// TODO: visibility

		Data(engine::graphics::data::Color color, std::string display)
			: color(color)
			, display(display)
		{}

		Data(engine::graphics::data::Color color)
			: color(color)
		{}

		Data(engine::Asset texture)
			: texture(texture)
		{}
	};

	using Datas = std::vector<std::pair<engine::Asset, Data>>;

	void show(engine::Asset window);

	void hide(engine::Asset window);

	void toggle(engine::Asset window);

	void select(engine::Asset window);

	void update(
		engine::Asset window,
		Datas && datas);

	void update(
		engine::Asset window,
		core::maths::Vector3f delta);
}
}

#endif // ENGINE_GUI_VIEWS_HPP
