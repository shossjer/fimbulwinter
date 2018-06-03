
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

			struct tab_t
			{
				Entity target;

				tab_t(Entity target)
					: target(target)
				{}
			};
		}
	}
}

#endif // ENGINE_GUI_INTERACTION_HPP
