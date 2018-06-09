
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_CONTROLLER_HPP
#define ENGINE_GUI_CONTROLLER_HPP

#include <engine/Entity.hpp>

#include "loading.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
		namespace controller
		{
			struct list_t
			{
				DataVariant item_template;

				View & view;
				View::Group & group;

				list_t(const DataVariant & item_template, View & view, View::Group & group)
					: item_template(item_template)
					, view(view)
					, group(group)
				{}
			};

			struct tab_t
			{
				// pager; group view where the tab pages are shown
				View & pager_view;

				// active tab - should never be null
				View * active_tab;

				// pages - only one added to pager at the time
				std::vector<View *> pages;

				// vector of all tab base views (to select / deselect)
				std::vector<View *> tabs;

				tab_t(View & pager_view)
					: pager_view(pager_view)
				{}
			};
		}
	}
}

#endif // ENGINE_GUI_CONTROLLER_HPP
