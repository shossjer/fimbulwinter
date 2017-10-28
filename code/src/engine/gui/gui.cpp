
#include "gui.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "measure.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>

#include <vector>

using namespace engine::gui;

namespace
{
	core::container::UnorderedCollection
		<
		engine::Entity, 100,
		std::array<Function, 100>,
		std::array<View, 100>
		>
		components;

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
				GroupData & data = utility::get<GroupData>(window_data);

				//auto & window = windows.emplace<Window>(
				//	data.name,
				//	data.name,
				//	data.size,
				//	data.layout,
				//	data.margin);

				//Creator(lookup, window).create_views(window.group, data);

				//window.init_window();
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
