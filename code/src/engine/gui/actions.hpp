
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_ACTIONS_HPP
#define ENGINE_GUI_ACTIONS_HPP

#include "gui.hpp"

#include <engine/Entity.hpp>

namespace engine
{
	namespace gui
	{
		struct CloseAction
		{
			Entity target;
		};
		struct InteractionAction
		{
			Entity target;
		};
		struct TriggerAction
		{
			Entity target;
		};
	}
}

#endif
