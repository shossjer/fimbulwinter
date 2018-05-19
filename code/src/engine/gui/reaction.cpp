
#include "reaction.hpp"

#include "common.hpp"
#include "update.hpp"

#include "utility/variant.hpp"

namespace engine
{
	namespace gui
	{
		extern Resources resources;

		extern void create(const Resources & resources, View & screen_view, View::Group & screen_group, const DataVariant & windows_data);
	}
}
namespace
{
	using namespace engine::gui;

	bool contains(const engine::Asset & key, const node_map_t & map)
	{
		return map.nodes.find(key) != map.nodes.end();
	}

	node_t & find(const engine::Asset & key, node_map_t & map)
	{
		return map.nodes.find(key)->second;
	}

	class SetupLookup
	{
	public:
		node_t operator() (data::Values & values)
		{
			debug_assert(!values.data.empty());

			return node_t{ utility::in_place_type<node_list_t>, std::move(values.data[0]) };
		}
		node_t operator() (data::KeyValues & keyValues)
		{
			node_t node{ utility::in_place_type<node_map_t> };
			auto & content = utility::get<node_map_t>(node);

			for (auto & keyVal : keyValues.data)
			{
				content.nodes.emplace(keyVal.first, visit(*this, keyVal.second));
			}

			return node;
		}
		node_t operator() (std::string & data)
		{
			return node_t{ utility::in_place_type<node_text_t> };
		}
		template<typename T>
		node_t operator() (const T & data)
		{
			debug_unreachable();
		}
	};

	void update_observer_number(ViewData & data, const std::size_t index)
	{
		if (!data.has_reaction())
			return;

		for (unsigned i = 0; i < data.reaction.observe.size(); i++)
		{
			if (data.reaction.observe[i].key == engine::Asset{ "*" })
			{
				data.reaction.observe[i].index = index;
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
					update_observer_number(visit(BaseData{ index }, child), index);
				}
				return data;
			}
			ViewData & operator() (PanelData & data) { return data; }
			ViewData & operator() (TextData & data) { return data; }
			ViewData & operator() (TextureData & data) { return data; }
		};

		update_observer_number(visit(BaseData{ index }, data), index);
	}

	void react(const reaction_list_t & reaction, const data::Values & values)
	{
		auto & view = reaction.controller->view;
		auto & group = reaction.controller->group;

		auto current_size = group.children.size();
		auto goal_size = values.data.size();

		if (current_size == goal_size)
			return;

		if (current_size < goal_size)
		{
			for (std::size_t i = current_size; i < goal_size; i++)
			{
				// create a copy which we will replace any list index in
				auto copy = reaction.controller->item_template;
				update_observer_number(copy, i);
				create(resources, view, group, copy);
			}
			ViewUpdater::creation(view);
		}
		else
		{
			// hide item views if needed (if the update contains less items than previously)
			const std::size_t items_remove = current_size - goal_size;

			for (std::size_t i = current_size - items_remove; i < current_size; i++)
			{
				ViewUpdater::hide(*reaction.controller->group.children[i]);
			}
		}
	}

	void react(const reaction_text_t & reaction, const std::string & data)
	{
		View::Text & content = utility::get<View::Text>(reaction.view->content);
		content.display = data;

		auto change = ViewUpdater::update(*reaction.view, content);
		ViewUpdater::parent(*reaction.view, change);
	}
}

namespace engine
{
	namespace gui
	{
		void clear(Reactions & reactions)
		{
			struct
			{
				void operator() (node_list_t & list)
				{
					list.reactions.clear();

					for (auto & node : list.nodes)
					{
						visit(*this, node);
					}
				}
				void operator() (node_map_t & map)
				{
					for (auto & node : map.nodes)
					{
						visit(*this, node.second);
					}
				}
				void operator() (node_text_t & node)
				{
					node.reactions.clear();
				}

			} lookup;

			lookup(reactions);
		}

		void setup(MessageDataSetup & message, Reactions & reactions)
		{
			debug_assert(!contains(message.data.first, reactions));

			reactions.nodes.emplace(message.data.first, visit(SetupLookup{}, message.data.second));
		}

		void update(MessageData & message, Reactions & reactions, Views & views)
		{
			debug_assert(contains(message.data.first, reactions));

			struct Lookup
			{
				node_t & node;

				void operator() (const data::Values & values)
				{
					node_list_t & list = utility::get<node_list_t>(node);

					// create nodes if list has expanded
					for (int i = list.nodes.size(); i < values.data.size(); i++)
					{
						list.nodes.push_back(visit(SetupLookup{}, list.item_setup));
					}

					// inform reactions to allow them to expand view's
					// this is where list controllers create views
					for (auto & reaction : list.reactions)
					{
						react(reaction, values);
					}

					// update all nodes in list after items are created
					for (int i = 0; i < values.data.size(); i++)
					{
						visit(Lookup{ list.nodes[i] }, values.data[i]);
					}
				}
				void operator() (const data::KeyValues & keyValues)
				{
					node_map_t & map = utility::get<node_map_t>(node);

					for (auto & keyValue : keyValues.data)
					{
						visit(Lookup{ find(keyValue.first, map) }, keyValue.second);
					}
				}
				void operator() (const std::string & data)
				{
					node_text_t & text = utility::get<node_text_t>(node);

					for (auto & reaction : text.reactions)
					{
						react(reaction, data);
					}
				}
				void operator() (const std::nullptr_t & data)
				{
					// TODO: act on
				}
			};

			visit(Lookup{ find(message.data.first, reactions) }, message.data.second);
		}
	}
}
