
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_INTERACTION_HPP
#define ENGINE_GUI_INTERACTION_HPP

#include "gui.hpp"

#include <engine/Entity.hpp>

namespace engine
{
	namespace gui
	{
		namespace action
		{
			struct close_t
			{
				Entity target;

				close_t(Entity target)
					: target(target)
				{}
			};

			struct selection_t
			{
				Entity target;

				selection_t(Entity target)
					: target(target)
				{}
			};

			// tab_t is only created automatically as part of controller::tab_t.
			// it is never specified in json files.
			struct tab_t
			{
				// base view of the clickable tab
				Entity target;

				// id of the tab controller
				Entity controller_id;

				tab_t(Entity target, Entity controller_id)
					: target(target)
					, controller_id(controller_id)
				{}
			};
		}
	}
}

#endif // ENGINE_GUI_INTERACTION_HPP
