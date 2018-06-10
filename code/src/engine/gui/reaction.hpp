
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_REACT_HPP
#define ENGINE_GUI_REACT_HPP

#include "gui.hpp"

#include <unordered_map>
#include <vector>

namespace engine
{
	namespace gui
	{
		class View;

		namespace controller
		{
			struct list_t;
			//struct tab_t;
		}

		struct reaction_list_t
		{
			// handle to list-controller
			controller::list_t & controller;

			reaction_list_t(controller::list_t & controller)
				: controller(controller)
			{}
		};
		struct reaction_text_t
		{
			View & view;

			reaction_text_t(View & view)
				: view(view)
			{}
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
			// replace with vector
			std::vector<std::pair<engine::Asset, node_t>> nodes;
		};

		struct node_text_t
		{
			std::vector<reaction_text_t> reactions;
		};
	}
}

#endif // ENGINE_GUI_REACT_HPP
