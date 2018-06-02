
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
			struct interaction_t
			{
				Entity target;

				interaction_t(Entity target)
					: target(target)
				{}
			};
		}
	}
}

#endif // ENGINE_GUI_INTERACTION_HPP
