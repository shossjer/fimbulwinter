
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_REACT_HPP
#define ENGINE_GUI_REACT_HPP

#include "gui.hpp"

#include "common.hpp"
#include "creation.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "render.hpp"
#include "update.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
		namespace react
		{
			void update_size(View & parent, Function::List & list, const std::vector<std::string> & data)
			{
				auto & parent_group = ViewUpdater::content<View::Group>(parent);

				if (data.size() == parent_group.children.size())
					return;

				// create item views if needed
				if (list.items.size() < data.size())
				{
					auto creator = Creator::instantiate();

					// create item views to match the size
					for (std::size_t i = list.items.size(); i < data.size(); i++)
					{
						auto & item = creator.create(&parent, &parent_group, list.item_template);
						list.items.emplace_back(&item);
					}
					ViewUpdater::view(parent, Change{ Change::DATA | Change::SIZE_HEIGHT | Change::SIZE_WIDTH });
				}
				// hide item views if needed (if the update contains less items than previously)
				else if (parent_group.children.size() > data.size())
				{
					auto & group = ViewUpdater::content<View::Group>(parent);
					const std::size_t items_remove = parent_group.children.size() - data.size();

					for (std::size_t i = parent_group.children.size() - items_remove; i < parent_group.children.size(); i++)
					{
						ViewUpdater::hide(*list.items[i]);
					}
				}
			}
			void update_data(View & parent, Function::List & list, const std::vector<std::string> & data)
			{
				// update the view data
				for (std::size_t i = 0; i < data.size(); i++)
				{
					// TODO: actually update data....
					auto & view = *list.items[i];

					if (utility::holds_alternative<View::Text>(view.content))
					{
						auto & content = ViewUpdater::content<View::Text>(view);

						content.display = data[i];

						auto change = ViewUpdater::update(view, content);
						ViewUpdater::parent(view, change);
					}
				}
			}

			// temp to test list
			void update(View & parent, Function::List & list, const std::vector<std::string> & data)
			{
				update_size(parent, list, data);
				update_data(parent, list, data);
			}

			struct TextUpdate
			{
				View & view;

				void operator() (const TextData & data)
				{
					auto & content = ViewUpdater::content<View::Text>(view);

					content.display = data.display;

					auto change = ViewUpdater::update(view, content);
					ViewUpdater::parent(view, change);
				}
				template<typename T>
				void operator() (const T &) { debug_unreachable(); }
			};
		}
	}
}

#endif // ENGINE_GUI_REACT_HPP
