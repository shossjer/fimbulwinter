
#ifndef TEST_ENGINE_GUI_GUI_ACCESS_HPP
#define TEST_ENGINE_GUI_GUI_ACCESS_HPP

#include <engine/gui/update.hpp>
#include <engine/gui/view.hpp>

namespace engine
{
	namespace gui
	{
		struct ViewAccess
		{
			template<typename T>
			static constexpr T & content(View & view)
			{
				return utility::get<T>(view.content);
			}

			static Change & change(View & view) { return view.change; }

			static Offset & offset(View & view) { return view.offset; }

			static Size & size(View & view) { return view.size; }

			static View create_group(
				View::Group::Layout layout,
				Size size,
				View * parent = nullptr,
				Margin margin = Margin{})
			{
				return View{
					Entity::create(),
					View::Content{ utility::in_place_type<View::Group>, layout },
					Gravity{},
					margin,
					size,
					parent };
			}
			static View create_child(
				View::Content && content,
				Size size,
				View * parent = nullptr,
				Margin margin = Margin{},
				Gravity gravity = Gravity{})
			{
				return View{
					Entity::create(),
					std::move(content),
					gravity,
					margin,
					size,
					parent };
			}
		};
	}
}

using namespace engine::gui;

#endif // !TEST_ENGINE_GUI_GUI_ACCESS_HPP
