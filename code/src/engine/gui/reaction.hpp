
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_REACT_HPP
#define ENGINE_GUI_REACT_HPP

#include "gui.hpp"

#include "view.hpp"

#include <unordered_map>
#include <vector>

namespace engine
{
	namespace gui
	{
		struct reaction_list_t
		{
			// handle to list-controller
		};
		struct reaction_text_t
		{
			View * view;
		};

		struct node_list_t;
		struct node_map_t;
		struct node_text_t;

		using node_t = utility::variant
		<
			node_list_t,
			node_map_t,
			node_text_t
		>;

		struct node_list_t
		{
			// used to "setup" list items when it is expanded
			data::Value item_setup;

			// directly interested in "list" updates (so lists...)
			std::vector<reaction_list_t> reactions;

			// node at each index
			std::vector<node_t> nodes;

			node_list_t(data::Value && item)
				: item_setup(std::move(item))
			{}
		};

		struct node_map_t
		{
			std::unordered_map<engine::Asset, node_t> nodes;
		};

		struct node_text_t
		{
			std::vector<reaction_text_t> reactions;
		};
	}
}

#endif // ENGINE_GUI_REACT_HPP
