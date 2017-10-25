
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
				Margin margin = Margin{})
			{
				return View{
					std::move(content),
					Gravity{},
					margin,
					size,
					parent };
			}
		};
	}
}

using namespace engine::gui2;

#endif // !TEST_ENGINE_GUI2_GUI_ACCESS_HPP
