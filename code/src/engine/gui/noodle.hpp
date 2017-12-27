
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_NOODLE_HPP
#define ENGINE_GUI_NOODLE_HPP

#include "gui.hpp"

namespace engine
{
	namespace gui
	{
		namespace data
		{
			struct Node;
			using Nodes = std::vector< std::pair<Asset, Node> >;

			struct Node
			{
				enum
				{
					MAP,
					LIST,
					TEXT
				}
				type;

				Nodes nodes;
				std::vector<Entity> targets;
			};

			// registers "data update" reaction hierarchy;
			// so reactions can be registered.
			struct ParseSetup
			{
				Asset key;
				Nodes & nodes;

				void operator() (const data::Values & values)
				{
					debug_assert(!values.data.empty());

					nodes.emplace_back(key, Node{ Node::LIST });

					// register one or more list items
					for (int i = 0; i < values.data.size(); i++)
					{
						visit(ParseSetup{
							Asset{ std::to_string(i) },
							nodes.back().second.nodes },
							values.data[i]);
					}
				}
				void operator() (const data::KeyValues & map)
				{
					nodes.emplace_back(key, Node{ Node::MAP });

					for (auto & data : map.data)
					{
						visit(ParseSetup{ data.first, nodes.back().second.nodes }, data.second);
					}
				}
				void operator() (const std::nullptr_t)
				{
					debug_unreachable(); // not valid for setup
				}
				void operator() (const std::string & data)
				{
					nodes.emplace_back(key, Node{ Node::TEXT });
				}
			};
		}
	}
}

#endif // ENGINE_GUI_NOODLE_HPP
