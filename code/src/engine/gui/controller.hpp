
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_CONTROLLER_HPP
#define ENGINE_GUI_CONTROLLER_HPP

#include <engine/Entity.hpp>

#include "loading.hpp"
#include "view.hpp"

namespace engine
{
	namespace gui
	{
		namespace controller
		{
			struct list_t
			{
				DataVariant item_template;

				View & view;
				View::Group & group;

				list_t(const DataVariant & item_template, View & view, View::Group & group)
					: item_template(item_template)
					, view(view)
					, group(group)
				{}
			};

			struct tab_t
			{

			};
		}
	}
}

#endif // ENGINE_GUI_CONTROLLER_HPP
