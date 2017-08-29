
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_ACTIONS_HPP
#define ENGINE_GUI_ACTIONS_HPP

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
#include <engine/Asset.hpp>

namespace engine
{
namespace gui
{
	class View;

	struct CloseAction
	{
		engine::Asset window;
		View * target;

		CloseAction(engine::Asset window, View * target)
			: window(window)
			, target(target)
		{}
	};

	struct InteractionAction
	{
		engine::Asset window;
		View * target;

		InteractionAction(engine::Asset window, View * target)
			: window(window)
			, target(target)
		{}
	};

	struct MoveAction
	{
		engine::Asset window;
	};

	struct SelectAction
	{
		engine::Asset window;
	};
}
}

#endif // ENGINE_GUI_ACTIONS_HPP
