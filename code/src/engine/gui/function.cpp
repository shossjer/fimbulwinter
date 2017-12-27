
#include "function.hpp"

#include "update.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
		struct Contains
		{
			Entity entity;
			View * view;

			template<typename T>
			bool operator() (T & content)
			{
				return view->entity == entity;
			}
			bool operator() (View::Group & content)
			{
				if (view->entity == entity)
					return true;

				for (auto child : content.children)
				{
					view = child;
					if (visit(*this, child->content))
						return true;
				}

				return false;
			}
		};

		namespace fun
		{
			void Trigger::operator() (Function::TabCantroller & content)
			{
				std::size_t clicked_index = content.active_index;
				for (std::size_t i = 0; i < content.tab_groups.size(); i++)
				{
					View * tab_group = content.tab_groups[i];
					if (visit(Contains{ this->caller, tab_group }, tab_group->content))
					{
						clicked_index = i;
						break;
					}
				}

				if (content.active_index == clicked_index)
					return;

				// hide / unselect the prev. tab
				ViewUpdater::status(
					*content.tab_groups[content.active_index],
					Status::DEFAULT);
				ViewUpdater::hide(
					*content.page_groups[content.active_index]);

				// show / select the new tab.
				ViewUpdater::status(
					*content.tab_groups[clicked_index],
					Status::PRESSED);
				ViewUpdater::show(
					*content.page_groups[clicked_index]);

				content.active_index = clicked_index;
			}
		}
	}
}
