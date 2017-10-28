
#ifndef ENGINE_GUI_GUI_HPP
#define ENGINE_GUI_GUI_HPP

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>
#include <core/maths/Vector.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui
	{
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

		void post_update_translate(
			engine::Asset window,
			core::maths::Vector3f delta);
	}
}

#endif
