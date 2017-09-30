
#include "measure.hpp"
#include "resources.hpp"
#include "updater.hpp"
#include "views.hpp"

namespace engine
{
namespace gui
{
	constexpr change_t::value_t change_t::NONE;
	constexpr change_t::value_t change_t::INITIAL;
	constexpr change_t::value_t change_t::MOVE;
	constexpr change_t::value_t change_t::SIZE;
	constexpr change_t::value_t change_t::VISIBILITY;

	value_t View::height() const
	{
		return this->margin.height() + this->size.height.value;
	}
	value_t View::width() const
	{
		return this->margin.width() + this->size.width.value;
	}

	View * View::find(const engine::Asset name)
	{
		return this->name == name ? this : nullptr;
	}
	View * View::find(const engine::Entity entity)
	{
		return this->entity == entity ? this : nullptr;
	}

	change_t View::refresh()
	{
		for (auto p : this->children)
		{
			this->change.set(p->refresh());
		}

		// measure view if needed
		ViewMeasure::measure_active(*this);

		return this->change;
	}

	Matrix4x4f Drawable::render_matrix() const
	{
		return make_translation_matrix(this->offset);
	}

	core::maths::Vector2f Drawable::render_size() const
	{
		return core::maths::Vector2f{
			static_cast<float>(this->size.width.value),
			static_cast<float>(this->size.height.value) };
	}

	void Drawable::translate(unsigned & order)
	{
		this->order_depth = static_cast<float>(order) / 100.f;
		order++;
	}

	void Drawable::refresh_changes(const Group * const group)
	{
		ViewMeasure::measure_passive(*this, group);
		refresh_renderer();
	}

	void Drawable::refresh_changes(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent)
	{
		ViewMeasure::measure_passive_forced(*this, size_parent, gravity_mask_parent, offset_parent);

		// TODO: have it here or in ViewMeasure?
		const Vector3f ret = ViewMeasure::offset(*this, size_parent, gravity_mask_parent, offset_parent);
		this->offset = ret + Vector3f{ 0.f, 0.f, this->order_depth };

		refresh_renderer();
	}

	void Drawable::refresh_renderer()
	{
		if (!this->change.any()) return;

		if (this->should_render != this->is_rendered)
		{
			if (this->should_render)	// check if view should be shown or hidden
			{
				renderer_send_add();

				if (is_selectable())
				{
					engine::graphics::renderer::post_make_selectable(this->entity);
				}
				this->is_rendered = true;
			}
			else
			{
				engine::graphics::renderer::post_remove(this->entity);
				this->is_rendered = false;
			}
		}
		else
		{
			// should not be "dirty" while not rendered
			debug_assert(this->should_render);
			renderer_send_update();
		}

		this->change.clear();
	}

	void PanelC::renderer_send_add() const
	{
		engine::graphics::renderer::post_add_panel(
			this->entity,
			engine::graphics::data::ui::PanelC{
				this->render_matrix(),
				this->render_size(),
				this->color->get(this) });
	}
	void PanelC::renderer_send_update() const
	{
		engine::graphics::renderer::post_update_panel(
			this->entity,
			engine::graphics::data::ui::PanelC{
				this->render_matrix(),
				this->render_size(),
				this->color->get(this) });
	}
	value_t PanelC::wrap_height() const
	{
		debug_printline(0xffffffff, "GUI - Wrap not possible for Color!");
		debug_unreachable();
	}
	value_t PanelC::wrap_width() const
	{
		debug_printline(0xffffffff, "GUI - Wrap not possible for Color!");
		debug_unreachable();
	}

	void PanelT::renderer_send_add() const
	{
		engine::graphics::renderer::post_add_panel(
			this->entity,
			engine::graphics::data::ui::PanelT{
				this->render_matrix(),
				this->render_size(),
				this->texture });
	}
	void PanelT::renderer_send_update() const
	{
		engine::graphics::renderer::post_update_panel(
			this->entity,
			engine::graphics::data::ui::PanelT{
				this->render_matrix(),
				this->render_size(),
				this->texture });
	}
	value_t PanelT::wrap_height() const
	{
		debug_printline(0xffffffff, "GUI - Wrap not implemented for Texture");
		debug_unreachable();
	}
	value_t PanelT::wrap_width() const
	{
		debug_printline(0xffffffff, "GUI - Wrap not implemented for Texture");
		debug_unreachable();
	}

	void Text::renderer_send_add() const
	{
		engine::graphics::renderer::post_add_text(
			this->entity,
			engine::graphics::data::ui::Text{
				this->render_matrix(),
				this->color->get(this),
				this->display });
	}
	void Text::renderer_send_update() const
	{
		engine::graphics::renderer::post_update_text(
			this->entity,
			engine::graphics::data::ui::Text{
				this->render_matrix(),
				this->color->get(this),
				this->display });
	}
	value_t Text::wrap_height() const
	{
		return 6;
	}
	value_t Text::wrap_width() const
	{
		return this->display.length() * value_t { 6 };
	}

	View * Group::find(const engine::Asset name)
	{
		if (this->name == name)
			return this;

		for (auto vp : this->children)
		{
			auto fp = vp->find(name);
			if (fp != nullptr)
				return fp;
		}

		return nullptr;
	}
	View * Group::find(const engine::Entity entity)
	{
		if (this->entity == entity)
			return this;

		for (auto vp : this->children)
		{
			auto fp = vp->find(entity);
			if (fp != nullptr)
				return fp;
		}

		return nullptr;
	}

	void Group::arrange_children(Vector3f offset)
	{
		switch (this->layout)
		{
		case Layout::HORIZONTAL:
		{
			Size playground = this->size;
			const Gravity mask{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

			for (auto p : this->children)
			{
				p->refresh_changes(playground, mask, offset);
				const auto ps = p->width();
				offset += Vector3f(static_cast<float>(ps), 0.f, 0.f);
				playground.width.value -= ps;
			}

			break;
		}
		case Layout::VERTICAL:
		{
			Size playground = this->size;
			const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

			for (auto p : this->children)
			{
				p->refresh_changes(playground, mask, offset);
				const auto ps = p->height();
				offset += Vector3f(0.f, static_cast<float>(ps), 0.f);
				playground.height.value -= ps;
			}

			break;
		}
		case Layout::RELATIVE:
		default:
		{
			const Gravity mask = Gravity::unmasked();

			for (auto p : this->children)
			{
				p->refresh_changes(this->size, mask, offset);
			}

			break;
		}
		}
	}

	void Group::translate(unsigned & order)
	{
		for (auto child : this->children)
		{
			child->translate(order);
		}
	}

	void Group::refresh_changes(const Group *const parent)
	{
		if (!this->change.any()) return;

		if (!this->change.affects_size())
		{
			// content has been updated of child view - does not change size
			for (auto & child : this->children)
			{
				child->refresh_changes(this);
			}
		}
		else
		{
			// Scenario:
			// GroupA{FIXED} -> GroupB{PARENT} -> GroupC{WRAP} -> Text{WRAP}
			//	 Refresh1:
			//	   Text is changed -> GroupC is changed
			//	   GroupB is unchanged
			//   Refresh2:
			//	   GroupA update{1}
			//	   GroupB update{1}
			//	   GroupC update{2} this case!
			ViewMeasure::measure_passive_forced(
				*this, parent->size, parent->gravity, parent->offset);

			arrange_children(
				ViewMeasure::offset(*this, parent->size, parent->gravity, parent->offset));
		}

		this->is_rendered = this->should_render;
		this->change.clear();
	}

	void Group::refresh_changes(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent)
	{
		// called if "parent" has changed its size

		// TODO: REMOVE!
		if (this->is_invisible && !this->is_rendered)
		{
			this->change.clear();
			return;
		}

		ViewMeasure::measure_passive_forced(
			*this, size_parent, gravity_mask_parent, offset_parent);

		arrange_children(
			ViewMeasure::offset(*this, size_parent, gravity_mask_parent, offset_parent));

		this->is_rendered = this->should_render;
		this->change.clear();
	}

	value_t Group::wrap_height() const
	{
		value_t val = 0;
		switch (layout)
		{
		case Layout::VERTICAL:
			for (auto child : children)
			{
				// TODO: if a child is "Size::PARENT" perhaps it should fill remaining view?
				const auto & s = child->get_size().height;
				if (s == Size::TYPE::FIXED || s == Size::TYPE::WRAP)
				{
					val += child->height();
				}
			}
			break;
		default:
			for (auto child : children)	// find "tallest" view
			{
				const auto & s = child->get_size().height;
				if (s == Size::TYPE::FIXED || s == Size::TYPE::WRAP)
				{
					auto h = child->height();
					if (val < h) val = h;
				}
			}
			break;
		}
		return val;
	}
	value_t Group::wrap_width() const
	{
		value_t val = 0;
		switch (layout)
		{
		case Layout::HORIZONTAL:
			for (auto child : children)
			{
				// TODO: if a child is "Size::PARENT" perhaps it should fill remaining view?
				const auto & s = child->get_size().width;
				if (s == Size::TYPE::FIXED || s == Size::TYPE::WRAP)
				{
					val += child->width();
				}
			}
			break;
		default:
			for (auto child : children)	// find "tallest" view
			{
				const auto & s = child->get_size().width;
				if (s == Size::TYPE::FIXED || s == Size::TYPE::WRAP)
				{
					auto w = child->width();
					if (val < w) val = w;
				}
			}
			break;
		}
		return val;
	}

	void List::translate(unsigned & order)
	{
		for (auto child : this->children)
		{
			child->translate(order);
		}

		// this local order is used when list dynamically creates more views.
		this->order = order;
	}

	void Window::refresh_window()
	{
		debug_assert(this->dirty);

		if (this->group.is_rendered || this->group.should_render)
		{
			this->group.refresh();
			this->group.refresh_changes(&this->group);
		}

		this->dirty = false;
	}

	void Window::show_window()
	{
		if (is_shown()) return;

		const Vector3f delta{ 0.f, 0.f, static_cast<float>(-(this->order)) };

		// move windows order to first when shown
		this->order = 0;
		this->position += delta;

		ViewUpdater::renderer_add(this->group);
		ViewUpdater::translate(this->group, delta);
		this->shown = true;
		this->dirty = true;
	}

	void Window::hide_window()
	{
		debug_assert(is_shown());
		ViewUpdater::renderer_remove(this->group);
		this->shown = false;
		this->dirty = true;
	}

	void Window::init_window()
	{
		unsigned view_order = 0;
		this->group.translate(view_order);
	}

	void Window::reorder_window(const int window_order)
	{
		if (!is_shown()) return;
		if (this->order == window_order) return;

		const float delta_depth = static_cast<float>(window_order - this->order);

		this->order = window_order;

		translate_window(Vector3f{ 0.f, 0.f, delta_depth });
	}

	void Window::translate_window(const Vector3f delta)
	{
		this->position += delta;

		ViewUpdater::translate(this->group, delta);

		if (is_shown()) this->dirty = true;
	}
}
}
