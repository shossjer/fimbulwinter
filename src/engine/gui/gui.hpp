
#ifndef ENGINE_GUI_GUI_HPP
#define ENGINE_GUI_GUI_HPP

#include <engine/Asset.hpp>
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
		std::vector<		// vector of list items
			std::vector<	// vector of item key value
				std::pair<engine::Asset, Data>>> list;
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

		Data(std::vector<std::vector<std::pair<engine::Asset, Data>>> && list)
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

	// can be called after "press" has been called. but a press does not always generate a click.
	void post_interaction_click(engine::Asset window, engine::Entity entity);

	void post_interaction_highlight(engine::Asset window, engine::Entity entity);

	// can only be called after highlight
	void post_interaction_lowlight(engine::Asset window, engine::Entity entity);

	// press will only be called on "highlighted" components
	// triggers "select" on window
	void post_interaction_press(engine::Asset window, engine::Entity entity);

	// can only be called after press", "click" can be triggered before release.
	void post_interaction_release(engine::Asset window, engine::Entity entity);

	//void post_interaction_select(engine::Asset window);

	void post_state_hide(engine::Asset window);

	void post_state_show(engine::Asset window);

	void post_state_toggle(engine::Asset window);

	void post_update_data(
		engine::Asset window,
		Datas && datas);

	void post_update_translate(
		engine::Asset window,
		core::maths::Vector3f delta);
}
}

#endif // ENGINE_GUI_VIEWS_HPP
