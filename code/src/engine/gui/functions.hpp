
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_FUNCTIONS_HPP
#define ENGINE_GUI_FUNCTIONS_HPP

#include <engine/Asset.hpp>

namespace engine
{
namespace gui
{
	class Group;
	class View;

	struct ProgressBar
	{
		engine::Asset window;
		View * target;

		enum Direction
		{
			HORIZONTAL,
			VERTICAL
		}
		direction;
	};

	struct TabBar
	{
		engine::Asset window;
		Group * base;
		Group * tab_container;
		Group * views_container;
		std::size_t active_index;
	};
}
}

#endif // ENGINE_GUI_FUNCTIONS_HPP
