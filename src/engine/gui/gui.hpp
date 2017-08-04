
#ifndef ENGINE_GUI_GUI_HPP
#define ENGINE_GUI_GUI_HPP

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
		enum Type
		{
			COLOR,
			DISPLAY,
			LIST,
			PROGRESS,
			TEXTURE
		} type;

		engine::graphics::data::Color color;
		std::string display;
		std::vector<std::pair<engine::Asset, Data>> list;
		float progress;
		engine::Asset texture;
		// TODO: visibility

		Data(std::string display)
			: type(DISPLAY)
			, display(display)
		{}

		Data(engine::graphics::data::Color color)
			: type(COLOR)
			, color(color)
		{}

		Data(std::vector<std::pair<engine::Asset, Data>> && list)
			: type(LIST)
			, list(std::move(list))
		{}

		Data(float progress)
			: type(PROGRESS)
			, progress(progress)
		{}

		explicit Data(engine::Asset texture)
			: type(TEXTURE)
			, texture(texture)
		{}
	};

	using Datas = std::vector<std::pair<engine::Asset, Data>>;

	void show(engine::Asset window);

	void hide(engine::Asset window);

	void toggle(engine::Asset window);

	void select(engine::Asset window);

	void trigger(engine::Entity entity);

	void update(
		engine::Asset window,
		Datas && datas);

	void update(
		engine::Asset window,
		core::maths::Vector3f delta);
}
}

#endif // ENGINE_GUI_VIEWS_HPP
