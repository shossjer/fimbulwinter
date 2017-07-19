
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
		std::string display;
		engine::graphics::data::Color color;
		engine::Asset texture;
		std::vector<std::pair<engine::Entity, Data>> list;
		// TODO: visibility
	};

	void update(
		engine::Asset window,
		std::vector<std::pair<engine::Entity, Data>> datas);

	void update(
		engine::Asset window,
		core::maths::Vector3f translation);
}
}

#endif // ENGINE_GUI_VIEWS_HPP
