
#ifndef ENGINE_GUI_GUI_HPP
#define ENGINE_GUI_GUI_HPP

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>
#include <core/maths/Vector.hpp>

#include <utility/variant.hpp>

#include <vector>

namespace engine
{
	namespace gui
	{
		struct MessageInteraction
		{
			engine::Entity entity;
			enum
			{
				CLICK,
				HIGHLIGHT,
				LOWLIGHT,
				PRESS,
				RELEASE
			}
			interaction;
		};

		struct MessageVisibility
		{
			engine::Asset window;
			enum
			{
				SHOW,
				HIDE,
				TOGGLE
			}
			state;
		};

		template<typename T>
		void post(T && data);
	}
}

#endif
