
#include "resources.hpp"
#include "views.hpp"

namespace engine
{
namespace gui
{
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

	void View::renderer_hide()
	{
		if (!this->should_render) return;
		this->should_render = false;
		this->is_dirty = true;
	}
	void View::renderer_show()
	{
		if (this->should_render) return;	// already/marked for rendering
		if (this->is_invisible) return;		// if invisible it cannot be rendered

		this->should_render = true;
		this->is_dirty = true;
	}
	void View::visibility_hide()
	{
		if (this->is_invisible) return; // already invisible

		if (this->is_rendered)	// set as dirty if shown
			this->is_dirty = true;

		this->is_invisible = true;
		this->should_render = false;
	}
	void View::visibility_show()
	{
		if (!this->is_invisible) return; // already visible

		this->is_invisible = false;
	}

	Vector3f View::arrange_offset(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent) const
	{
		Vector3f::array_type buff;
		offset_parent.get_aligned(buff);

		float offset_horizontal;

		if (this->gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_RIGHT))
		{
			offset_horizontal = buff[0] + size_parent.width.value - this->size.width.value - this->margin.right;
		}
		else
		if (this->gravity.place(gravity_mask_parent, Gravity::HORIZONTAL_CENTRE))
		{
			offset_horizontal = buff[0] + (size_parent.width.value / 2) - (this->size.width.value / 2);
		}
		else // LEFT default
		{
			offset_horizontal = buff[0] + this->margin.left;
		}

		float offset_vertical;

		if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_BOTTOM))
		{
			offset_vertical = buff[1] + size_parent.height.value - this->size.height.value - this->margin.bottom;
		}
		else
		if (this->gravity.place(gravity_mask_parent, Gravity::VERTICAL_CENTRE))
		{
			offset_vertical = buff[1] + (size_parent.height.value / 2) - (this->size.height.value / 2) + this->margin.top - this->margin.bottom;
		}
		else // TOP default
		{
			offset_vertical = buff[1] + this->margin.top;
		}

		return Vector3f(offset_horizontal, offset_vertical, buff[2]);
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

	void Drawable::update(const State state)
	{
		this->state = state;
		this->is_dirty = true;
	}

	void Drawable::translate(core::maths::Vector3f delta)
	{
		this->offset += delta;
		this->is_dirty = true;
	}
	void Drawable::translate(unsigned & order)
	{
		this->order_depth = static_cast<float>(order) / 100.f;
		order++;
	}

	void Drawable::refresh2(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent)
	{
		if (!this->is_dirty) return;	// no change

		{
			const auto max_height = size_parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				// do nothing
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENT:
				this->size.height.percentage(max_height);
				break;
			case Size::TYPE::WRAP:
				// do nothing
				break;
			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = size_parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				// do nothing
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENT:
				this->size.width.percentage(max_width);
				break;
			case Size::TYPE::WRAP:
				// do nothing
				break;
			default:
				debug_unreachable();
			}
		}

		const Vector3f ret = arrange_offset(size_parent, gravity_mask_parent, offset_parent);
		this->offset = ret + Vector3f{ 0.f, 0.f, this->order_depth };

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

		this->is_dirty = false;
	}

	unsigned PanelC::refresh1()
	{
		if (!this->is_dirty) return 0;

		switch (this->size.height.type)
		{
		case Size::TYPE::FIXED:
			this->size.height.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;

		case Size::TYPE::WRAP:
		default:
			debug_unreachable();
		}

		switch (this->size.width.type)
		{
		case Size::TYPE::FIXED:
			this->size.width.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;

		case Size::TYPE::WRAP:
		default:
			debug_unreachable();
		}

		return 1;
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

	unsigned PanelT::refresh1()
	{
		if (!this->is_dirty) return 0;

		switch (this->size.height.type)
		{
		case Size::TYPE::FIXED:
			this->size.height.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;

		case Size::TYPE::WRAP:
			// disable until we have access to TextureInfo object
		default:
			debug_unreachable();
		}

		switch (this->size.width.type)
		{
		case Size::TYPE::FIXED:
			this->size.width.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;

		case Size::TYPE::WRAP:
			// disable until we have access to TextureInfo object
		default:
			debug_unreachable();
		}

		return 1;
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

	unsigned Text::refresh1()
	{
		if (!this->is_dirty) return 0;

		switch (this->size.height.type)
		{
		case Size::TYPE::FIXED:
			this->size.height.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;
		case Size::TYPE::WRAP:
			this->size.height.value = 6;
			break;

		default:
			debug_unreachable();
		}

		switch (this->size.width.type)
		{
		case Size::TYPE::FIXED:
			this->size.width.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;
		case Size::TYPE::WRAP:
			this->size.width.value = this->display.length() * value_t { 6 };
			break;

		default:
			debug_unreachable();
		}

		return 1;
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

	unsigned Group::measure_children()
	{
		unsigned children_dirty = 0;
		Size size_children = this->size;

		switch (this->layout)
		{
		case Layout::HORIZONTAL:
		{
			for (auto p : this->children)
			{
				children_dirty |= p->refresh1();
				size_children.width -= p->width();
			}

			if (this->size.width == Size::TYPE::WRAP)
				this->size.width.wrap(size_children.width.value);

			break;
		}
		case Layout::VERTICAL:
		{
			for (auto p : this->children)
			{
				children_dirty |= p->refresh1();
				size_children.height -= p->height();
			}

			if (this->size.height == Size::TYPE::WRAP)
				this->size.height.wrap(size_children.height.value);

			break;
		}
		case Layout::RELATIVE:
		default:
		{
			for (auto p : this->children)
			{
				children_dirty |= p->refresh1();

				// used if this size is wrap
				size_children.width.value = std::max(size_children.width.value, p->width());
				size_children.height.value = std::max(size_children.height.value, p->width());
			}

			if (this->size.width == Size::TYPE::WRAP)
				this->size.width.wrap(size_children.width.value);

			if (this->size.height == Size::TYPE::WRAP)
				this->size.height.wrap(size_children.height.value);

			break;
		}
		}
		return children_dirty;
	}

	void Group::arrange_children(Vector3f offset)
	{
		switch (this->layout)
		{
		case Layout::HORIZONTAL:
		{
			const Gravity mask{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

			for (auto p : this->children)
			{
				p->refresh2(this->size, mask, offset);
				offset += Vector3f(static_cast<float>(p->width()), 0.f, 0.f);
			}

			break;
		}
		case Layout::VERTICAL:
		{
			const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

			for (auto p : this->children)
			{
				p->refresh2(this->size, mask, offset);
				offset += Vector3f(0.f, static_cast<float>(p->height()), 0.f);
			}

			break;
		}
		case Layout::RELATIVE:
		default:
		{
			const Gravity mask = Gravity::unmasked();

			for (auto p : this->children)
			{
				p->refresh2(this->size, mask, offset);
			}

			break;
		}
		}
	}

	void Group::renderer_hide()
	{
		if (!this->should_render) return;

		for (auto child : this->children)
		{
			child->renderer_hide();
		}

		this->should_render = false;
		this->is_dirty = true;
	}
	void Group::renderer_show()
	{
		if (this->should_render) return;	// already/marked for rendering
		if (this->is_invisible) return;		// if invisible it cannot be rendered

		for (auto child : this->children)
		{
			child->renderer_show();
		}

		this->should_render = true;
		this->is_dirty = true;
	}
	void Group::visibility_hide()
	{
		if (this->is_invisible) return; // already invisible

		for (auto child : this->children)
		{
			child->visibility_hide();
		}

		if (this->is_rendered)	// set as dirty if shown
			this->is_dirty = true;

		this->is_invisible = true;
		this->should_render = false;
	}
	void Group::visibility_show()
	{
		if (!this->is_invisible) return; // already visible

		for (auto child : this->children)
		{
			child->visibility_show();
		}

		this->is_invisible = false;
	}

	void Group::update(const State state)
	{
		this->state = state;

		for (auto child : this->children)
		{
			child->update(state);
		}
	}

	void Group::translate(const Vector3f delta)
	{
		for (auto child : this->children)
		{
			child->translate(delta);
		}
	}

	void Group::translate(unsigned & order)
	{
		for (auto child : this->children)
		{
			child->translate(order);
		}
	}

	unsigned Group::refresh1()
	{
		switch (this->size.height.type)
		{
		case Size::TYPE::FIXED:
			this->size.height.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;
		case Size::TYPE::WRAP:
			// do nothing - yet
			break;

		default:
			debug_unreachable();
		}

		switch (this->size.width.type)
		{
		case Size::TYPE::FIXED:
			this->size.width.fixed();
			break;
		case Size::TYPE::PARENT:
		case Size::TYPE::PERCENT:
			// do nothing
			break;
		case Size::TYPE::WRAP:
			// do nothing - yet
			break;

		default:
			debug_unreachable();
		}

		auto r = measure_children();
		this->is_dirty = r > 0;
		return r;
	}

	void Group::refresh2(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent)
	{
		if (!this->is_dirty) return;

		{
			const auto max_height = size_parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				// do nothing
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENT:
				this->size.height.percentage(max_height);
				break;
			case Size::TYPE::WRAP:
				// do nothing
				break;
			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = size_parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				// do nothing
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENT:
				this->size.width.percentage(max_width);
				break;
			case Size::TYPE::WRAP:
				// do nothing
				break;
			default:
				debug_unreachable();
			}
		}

		Vector3f offset = arrange_offset(size_parent, gravity_mask_parent, offset_parent);

		arrange_children(offset);

		this->is_dirty = false;
		this->is_rendered = this->should_render;
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
			this->group.refresh1();
			this->group.arrange_children(
				this->position + Vector3f{ 0.f, 0.f, static_cast<float>(this->order) });
			this->group.is_rendered = this->group.should_render;
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

		this->group.renderer_show();
		this->group.translate(delta);

		this->shown = true;
		this->dirty = true;
	}

	void Window::hide_window()
	{
		debug_assert(is_shown());
		this->group.renderer_hide();
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

		this->group.translate(delta);

		if (is_shown()) this->dirty = true;
	}
}
}
