
#include "react.hpp"

#include "creation.hpp"
#include "loading.hpp"
#include "update.hpp"

namespace engine
{
	namespace gui
	{
		namespace react
		{
			void update(ViewData & data, const std::size_t index)
			{
				if (!data.has_reaction())
					return;

				for (unsigned i = 0; i < data.reaction.size(); i++)
				{
					if (data.reaction[i].key == engine::Asset{ "*" })
					{
						data.reaction[i].key = engine::Asset{ std::to_string(index) };
						break;
					}
				}
			}
			void update_observer_number(DataVariant & data, const std::size_t index)
			{
				struct BaseData
				{
					std::size_t index;

					ViewData & operator() (GroupData & data)
					{
						for (auto & child : data.children)
						{
							update(visit(BaseData{index}, child), index);
						}
						return data;
					}
					ViewData & operator() (PanelData & data) { return data; }
					ViewData & operator() (TextData & data) { return data; }
					ViewData & operator() (TextureData & data) { return data; }
				};

				update(visit(BaseData{index}, data), index);
			}
			void update_size(View & parent, Function::List & list, const std::vector<data::Value> & data)
			{
				auto & parent_group = ViewUpdater::content<View::Group>(parent);

				if (data.size() == parent_group.children.size())
					return;

				// create item views if needed
				if (list.items.size() < data.size())
				{
					auto creator = Creator::instantiate(parent.depth);

					// create item views to match the size
					for (std::size_t i = list.items.size(); i < data.size(); i++)
					{
						auto copy = list.item_template;
						update_observer_number(copy, i);
						auto & item = creator.create(&parent, &parent_group, copy);
						list.items.emplace_back(&item);
					}
					ViewUpdater::creation(parent);
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
		}

		void ListReaction::operator() (Function & function)
		{
			auto & parent = *function.view;
			auto & list = utility::get<Function::List>(function.content);

			react::update_size(parent, list, values.data);
		}

		void TextReaction::operator() (View & view)
		{
			auto & content = ViewUpdater::content<View::Text>(view);

			content.display = display;

			auto change = ViewUpdater::update(view, content);
			ViewUpdater::parent(view, change);
		}
	}
}
