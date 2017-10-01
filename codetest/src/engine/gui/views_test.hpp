
#ifndef TEST_ENGINE_GUI_VIEWS_TEST_HPP
#define TEST_ENGINE_GUI_VIEWS_TEST_HPP

#include <engine/gui/views.hpp>

namespace engine
{
	namespace gui
	{
		struct ViewTester
		{
			static change_t & change(View & view) { return view.change; }
			static Size & size(View & view) { return view.size; }

			static change_t::value_t flags(change_t & change) { return change.flags; }
			static std::size_t items(const List & view) { return view.shown_items; }

			static bool & should_render(View & view) { return view.should_render; }
			static bool & is_rendered(View & view) { return view.is_rendered; }
			static bool & is_invisible(View & view) { return view.is_invisible; }
		};

		class TestView : public View
		{
		public:
			const value_t size_wrap_h;
			const value_t size_wrap_w;

		private:
			TestView(Size size, value_t size_wrap_h, value_t size_wrap_w)
				: View(engine::Entity{}, engine::Asset{}, Gravity{}, Margin{}, size)
				, size_wrap_h(size_wrap_h)
				, size_wrap_w(size_wrap_w)
			{}
		public:
			TestView(value_t size_wrap_h, value_t size_wrap_w)
				: TestView(
					Size{ { Size::TYPE::WRAP },{ Size::TYPE::WRAP } },
					size_wrap_h,
					size_wrap_w)
			{}
			TestView(Size size)
				: TestView(size, 0, 0)
			{}
			TestView()
				: TestView(Size{ { Size::TYPE::PARENT },{ Size::TYPE::PARENT } })
			{}

			void translate(unsigned & order) override
			{}

			void refresh_changes(const Group *const parent) override
			{}

			void refresh_changes(
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent) override
			{}

			value_t wrap_height() const override { return this->size_wrap_h; }
			value_t wrap_width() const override { return this->size_wrap_w; }
		};

		class TestGroup : public Group
		{
		public:
			const value_t size_wrap_h;
			const value_t size_wrap_w;

		private:
			TestGroup(Size size, Layout layout, value_t size_wrap_h, value_t size_wrap_w)
				: Group(engine::Entity{}, engine::Asset{}, Gravity{}, Margin{}, size, layout)
				, size_wrap_h(size_wrap_h)
				, size_wrap_w(size_wrap_w)
			{}
		public:
			TestGroup(value_t size_wrap_h, value_t size_wrap_w)
				: TestGroup(
					Size{ { Size::TYPE::WRAP },{ Size::TYPE::WRAP } },
					Layout{},
					size_wrap_h,
					size_wrap_w)
			{}
			TestGroup(Size size, Layout layout)
				: TestGroup(size, layout, 0, 0)
			{}
			TestGroup()
				: TestGroup(Size{ { Size::TYPE::PARENT },{ Size::TYPE::PARENT } }, Layout{})
			{}

			void translate(unsigned & order) override
			{}

			void refresh_changes(const Group *const parent) override
			{}

			void refresh_changes(
				const Size size_parent,
				const Gravity gravity_mask_parent,
				const Vector3f offset_parent) override
			{}

			value_t wrap_height() const override { return this->size_wrap_h; }
			value_t wrap_width() const override { return this->size_wrap_w; }
		};
	}
}

using namespace engine::gui;

#endif // !TEST_ENGINE_GUI_VIEWS_TEST_HPP
