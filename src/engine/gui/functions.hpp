
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_FUNCTIONS_HPP
#define ENGINE_GUI_FUNCTIONS_HPP

/**
	the "actions" are stored in a separate collection since they
	share entity with the component they are created for.
	So a "close action" will have the entity of the view it was
	created for. this is because renderer will detect mouse cursor
	highlight/click on the registered "selectable" view; and will
	report this entity.

	the view of the action does not need to be "named". so it
	does not have to be registered in the "lookup" table.
 */
#include <engine/common.hpp>

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
