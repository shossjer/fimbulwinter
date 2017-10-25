
#ifndef TEST_ENGINE_GUI2_GUI_ACCESS_HPP
#define TEST_ENGINE_GUI2_GUI_ACCESS_HPP

#include <engine/gui2/update.hpp>
#include <engine/gui2/view.hpp>

namespace engine
{
	namespace gui2
	{
		struct ViewAccess
		{
			static Change & change(View & view) { return view.change; }

			static Size & size(View & view) { return view.size; }
		};
	}
}

using namespace engine::gui2;

#endif // !TEST_ENGINE_GUI2_GUI_ACCESS_HPP
