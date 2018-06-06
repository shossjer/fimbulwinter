
#include "controller.hpp"

#include "common.hpp"
#include "interaction.hpp"
#include "update.hpp"

namespace
{
	using namespace engine::gui;

	auto find_index(const View * view, const std::vector<View*> views)
	{
		auto itr = std::find(views.begin(), views.end(), view);
		debug_assert(itr != views.end()); // if itr == end then the object was not found :(
		return std::distance(views.begin(), itr);
	}
}

namespace engine
{
	namespace gui
	{
		namespace controller
		{
			void activate(Controllers & controllers, Views & views, action::tab_t & action)
			{
				auto & tab_controller = controllers.get<controller::tab_t>(action.controller_id);
				auto & selected_tab = views.get<View>(action.target);

				//  check if changed
				if (tab_controller.active_tab == &selected_tab)
					return;

				// find the index of the newly selected tab
				auto index = find_index(&selected_tab, tab_controller.tabs);

				// update tab selection - needs to be separate from HIGHLIGHT / LOWLIGHT!
				ViewUpdater::status(*tab_controller.active_tab, Status::State::DEFAULT);
				ViewUpdater::status(selected_tab, Status::State::PRESSED);

				// switch content
				ViewUpdater::hide(*tab_controller.active_tab);
				ViewUpdater::show(selected_tab);

				tab_controller.active_tab = &selected_tab;
			}
		}
	}
}
