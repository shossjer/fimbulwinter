
#include "reaction.hpp"

#include "common.hpp"
#include "update.hpp"

#include "utility/variant.hpp"

namespace
{
	using namespace engine::gui;

	bool contains(const engine::Asset & key, const std::unordered_map<engine::Asset, node_t> & nodes)
	{
		return nodes.find(key) != nodes.end();
	}

	node_t & find(const engine::Asset & key, std::unordered_map<engine::Asset, node_t> & nodes)
	{
		return nodes.find(key)->second;
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

	void react(const reaction_list_t & reaction, const data::Values & values)
	{
		// TODO: access use list controller to update group based on item template
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
		void setup(MessageDataSetup & message, Reactions & reactions)
		{
			debug_assert(!contains(message.data.first, reactions));

			reactions.emplace(message.data.first, visit(SetupLookup{}, message.data.second));
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
						visit(Lookup{ find(keyValue.first, map.nodes) }, keyValue.second);
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
