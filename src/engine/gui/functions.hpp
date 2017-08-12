
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_FUNCTIONS_HPP
#define ENGINE_GUI_FUNCTIONS_HPP

#include <engine/Asset.hpp>

namespace engine
{
namespace gui
{
	class View;

	struct ProgressBar
	{
		engine::Asset window;
		View * target;

		enum Direction
		{
			HORIZONTAL,
			VERTICAL
		} direction;

	};
}
}

#endif // ENGINE_GUI_FUNCTIONS_HPP
