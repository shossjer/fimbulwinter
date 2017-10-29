
#include "creation.hpp"
#include "gui.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "measure.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/CircleQueue.hpp>

#include <vector>

using namespace engine::gui;

namespace
{
	const uint32_t WINDOW_HEIGHT = 677; // 720
	const uint32_t WINDOW_WIDTH = 1004; // 1024

	struct Window
	{
		engine::Asset name;
		View * view;
	};

	core::container::Collection
		<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 1>
		>
		windows;

	Components components;

	// TODO: update lookup structure
	// init from "gameplay" data structures
	// assign views during creation
}

namespace engine
{
	namespace gui
	{
		extern std::vector<DataVariant> load();

		void create()
		{
			// load windows and views data from somewhere.
			std::vector<DataVariant> windows_data = load();

			for (auto & window_data : windows_data)
			{
				View & view = visit(Creator{ components }, window_data);

				// TODO: use "window size" as size param
				ViewMeasure::refresh(view, Gravity::unmasked(), Offset{}, Size{ {Size::FIXED, height_t{ WINDOW_HEIGHT } }, { Size::FIXED, width_t{ WINDOW_WIDTH } } });
				ViewMeasure::refresh(view);
			}
		}

		void destroy()
		{
			// TODO: destroy something
		}

		void update()
		{

		}

		void post_interaction_click(Asset window, Entity entity)
		{
		}

		void post_interaction_highlight(Asset window, Entity entity)
		{
		}

		void post_interaction_lowlight(Asset window, Entity entity)
		{
		}

		void post_interaction_press(Asset window, Entity entity)
		{
		}

		void post_interaction_release(Asset window, Entity entity)
		{
		}

		void post_state_hide(Asset window)
		{
		}

		void post_state_show(Asset window)
		{
		}

		void post_state_toggle(Asset window)
		{
		}

		void post_update_translate(Asset window, core::maths::Vector3f delta)
		{
		}
	}
}
