
#ifndef TEST_ENGINE_GUI_VIEWS_TEST_HPP
#define TEST_ENGINE_GUI_VIEWS_TEST_HPP

#include <engine/gui/views.hpp>

namespace engine
{
	namespace gui
	{
		struct ViewTester
		{
			static change_t::value_t flags(change_t & change) { return change.flags; }
			static change_t & change(View & view) { return view.change; }

			static std::size_t items(const List & view) { return view.shown_items; }


			static bool & should_render(View & view) { return view.should_render; }
			static bool & is_rendered(View & view) { return view.is_rendered; }
			static bool & is_invisible(View & view) { return view.is_invisible; }
		};

		class TestView : public View
		{
		public:
			TestView(Size size)
				: View(engine::Entity{}, engine::Asset{}, Gravity{}, Margin{}, size)
			{}
			TestView()
				: TestView(Size{ { Size::TYPE::FIXED, 100.f },{ Size::TYPE::WRAP } })
			{}

			void translate(unsigned & order) override
			{

			}

			change_t refresh1() override
			{
				return this->change;
			}

			void refresh2(const Group *const parent) override
			{

			}

			void refresh2(
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent) override
			{

			}
		};
	}
}

using namespace engine::gui;

#endif // !TEST_ENGINE_GUI_VIEWS_TEST_HPP
