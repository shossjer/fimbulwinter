
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

	void Drawable::update_translation(core::maths::Vector3f delta)
	{
		this->offset += delta;
	}

	auto Drawable::render_matrix() const
	{
		return make_translation_matrix(this->offset);
	}

	auto Drawable::render_size() const
	{
		return core::maths::Vector2f{
			static_cast<float>(this->size.width.value),
			static_cast<float>(this->size.height.value) };
	}

	void Drawable::arrange(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent,
		float & offset_depth)
	{
		const Vector3f ret = arrange_offset(size_parent, gravity_mask_parent, offset_parent);
		this->offset = ret + Vector3f{ 0.f, 0.f, offset_depth };
		offset_depth += DEPTH_INC;
	}

	void Drawable::hide()
	{
		// TODO: check if view is visible

		engine::graphics::renderer::post_remove(this->entity);
	}

	void Drawable::translate(core::maths::Vector3f delta)
	{
		// TODO: check if view is visible

		update_translation(delta);

		// send the updated matrix
		engine::graphics::renderer::post_update_modelviewmatrix(
			this->entity,
			engine::graphics::data::ModelviewMatrix{ render_matrix() });
	}

	void PanelC::measure(const Size parent)
	{
		{
			const auto max_height = parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				this->size.height.fixed(max_height, this->margin.height());
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.height.percentage(max_height);
				break;

			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				this->size.width.fixed(max_width, this->margin.width());
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.width.percentage(max_width);
				break;

			default:
				debug_unreachable();
			}
		}
	}

	void PanelC::show(const Vector3f delta)
	{
		// TODO: check that view is not hidden

		update_translation(delta);

		engine::graphics::renderer::post_add_panel(
			this->entity,
			engine::graphics::data::ui::PanelC{
				this->render_matrix(),
				this->render_size(),
				this->color });

		if (is_selectable())
		{
			engine::graphics::renderer::post_make_selectable(this->entity);
		}
	}

	void PanelC::refresh()
	{
		engine::graphics::renderer::post_update_panel(
			this->entity,
			engine::graphics::data::ui::PanelC{
				this->render_matrix(),
				this->render_size(),
				this->color });
	}

	void PanelT::measure(const Size parent)
	{
		{
			const auto max_height = parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				this->size.height.fixed(max_height, this->margin.height());
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.height.percentage(max_height);
				break;

			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				this->size.width.fixed(max_width, this->margin.width());
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.width.percentage(max_width);
				break;

			default:
				debug_unreachable();
			}
		}
	}

	void PanelT::show(const Vector3f delta)
	{
		// TODO: check that view is not hidden

		update_translation(delta);

		engine::graphics::renderer::post_add_panel(
			this->entity,
			engine::graphics::data::ui::PanelT{
				this->render_matrix(),
				this->render_size(),
				this->texture });

		if (is_selectable())
		{
			engine::graphics::renderer::post_make_selectable(this->entity);
		}
	}

	void PanelT::refresh()
	{
		engine::graphics::renderer::post_update_panel(
			this->entity,
			engine::graphics::data::ui::PanelT{
				this->render_matrix(),
				this->render_size(),
				this->texture });
	}

	void Text::measure(const Size parent)
	{
		{
			const auto max_height = parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				this->size.height.fixed(max_height, this->margin.height());
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.height.percentage(max_height);
				break;
			case Size::TYPE::WRAP:
				this->size.height.value = 6;
				break;
			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				this->size.width.fixed(max_width, this->margin.width());
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.width.percentage(max_width);
				break;
			case Size::TYPE::WRAP:
				this->size.width.value = this->display.length() * value_t { 6 };
				break;

			default:
				debug_unreachable();
			}
		}
	}

	void Text::show(const Vector3f delta)
	{
		// TODO: check that view is not hidden

		update_translation(delta);

		engine::graphics::renderer::post_add_text(
			this->entity,
			engine::graphics::data::ui::Text{
				this->render_matrix(),
				this->color,
				this->display });

		if (is_selectable())
		{
			engine::graphics::renderer::post_make_selectable(this->entity);
		}
	}

	void Text::refresh()
	{
		engine::graphics::renderer::post_update_text(
			this->entity,
			engine::graphics::data::ui::Text{
				this->render_matrix(),
				this->color,
				this->display });
	}

	void Group::measure_children()
	{
		Size size_children = this->size;

		switch (this->layout)
		{
		case Layout::HORIZONTAL:
		{
			for (auto p : this->children)
			{
				p->measure(size_children);
				size_children.width -= p->width();
			}

			if (this->size.width == Size::TYPE::WRAP)
				this->size.width -= size_children.width;

			break;
		}
		case Layout::VERTICAL:
		{
			for (auto p : this->children)
			{
				p->measure(size_children);
				size_children.height -= p->height();
			}

			if (this->size.height == Size::TYPE::WRAP)
				this->size.height -= size_children.height;

			break;
		}
		case Layout::RELATIVE:
		default:
		{
			for (auto p : this->children)
			{
				p->measure(this->size);

				// used if this size is wrap
				size_children.width.value = std::max(size_children.width.value, p->width());
				size_children.height.value = std::max(size_children.height.value, p->width());
			}

			if (this->size.width == Size::TYPE::WRAP)
				this->size.width.value = size_children.width.value;

			if (this->size.height == Size::TYPE::WRAP)
				this->size.height.value = size_children.height.value;

			break;
		}
		}
	}

	void Group::arrange_children(Vector3f offset, float & offset_depth)
	{
		switch (this->layout)
		{
		case Layout::HORIZONTAL:
		{
			const Gravity mask{ Gravity::VERTICAL_BOTTOM | Gravity::VERTICAL_CENTRE | Gravity::VERTICAL_TOP };

			for (auto p : this->children)
			{
				p->arrange(this->size, mask, offset, offset_depth);
				offset += Vector3f(static_cast<float>(p->width()), 0.f, 0.f);
			}

			break;
		}
		case Layout::VERTICAL:
		{
			const Gravity mask{ Gravity::HORIZONTAL_LEFT | Gravity::HORIZONTAL_CENTRE | Gravity::HORIZONTAL_RIGHT };

			for (auto p : this->children)
			{
				p->arrange(this->size, mask, offset, offset_depth);
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
				p->arrange(this->size, mask, offset, offset_depth);
			}

			break;
		}
		}
	}

	void Group::arrange(
		const Size size_parent,
		const Gravity gravity_mask_parent,
		const Vector3f offset_parent,
		float & offset_depth)
	{
		Vector3f offset = arrange_offset(size_parent, gravity_mask_parent, offset_parent);

		arrange_children(offset, offset_depth);
	}

	void Group::measure(const Size parent)
	{
		{
			const auto max_height = parent.height.value - this->margin.height();

			switch (this->size.height.type)
			{
			case Size::TYPE::FIXED:
				this->size.height.fixed(max_height, this->margin.height());
				break;
			case Size::TYPE::PARENT:
				this->size.height.parent(max_height);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.height.percentage(max_height);
				break;
			case Size::TYPE::WRAP:
				this->size.height.value = parent.height.value;
				break;

			default:
				debug_unreachable();
			}
		}
		{
			const auto max_width = parent.width.value - this->margin.width();

			switch (this->size.width.type)
			{
			case Size::TYPE::FIXED:
				this->size.width.fixed(max_width, this->margin.width());
				break;
			case Size::TYPE::PARENT:
				this->size.width.parent(max_width);
				break;
			case Size::TYPE::PERCENTAGE:
				this->size.width.percentage(max_width);
				break;
			case Size::TYPE::WRAP:
				this->size.width.value = parent.width.value;
				break;

			default:
				debug_unreachable();
			}
		}

		measure_children();
	}

	void Group::show(const Vector3f delta)
	{
		for (auto child : this->children)
		{
			child->show(delta);
		}
	}

	void Group::hide()
	{
		for (auto child : this->children)
		{
			child->hide();
		}
	}

	void Group::refresh()
	{
		for (auto child : this->children)
		{
			child->refresh();
		}
	}

	void Group::translate(const Vector3f delta)
	{
		for (auto child : this->children)
		{
			child->translate(delta);
		}
	}

	void Window::show_window()
	{
		if (this->shown) return;

		this->shown = true;

		const Vector3f delta{ 0.f, 0.f, static_cast<float>(-(this->order)) };

		// move windows order to first when shown
		this->order = 0;
		this->position += delta;

		this->group.show(delta);
	}

	void Window::hide_window()
	{
		debug_assert(this->shown);
		this->shown = false;

		this->group.hide();
	}

	void Window::measure_window()
	{
		Size & size = this->group.size;

		switch (size.height.type)
		{
		case Size::TYPE::FIXED:
			size.height.value = size.height.meta;
			break;

		default:
			debug_unreachable();
		}
		switch (size.width.type)
		{
		case Size::TYPE::FIXED:
			size.width.value = size.width.meta;
			break;

		default:
			debug_unreachable();
		}

		this->group.measure_children();

		float depth = static_cast<float>(this->order);
		this->group.arrange_children(this->position, depth);
	}

	void Window::update_window()
	{
		measure_window();

		if (is_shown())
		{
			this->group.refresh();
		}
	}

	void Window::reorder_window(const int window_order)
	{
		if (!this->shown) return;
		if (this->order == window_order) return;

		const float delta_depth = static_cast<float>(window_order - this->order);

		this->order = window_order;

		translate_window(Vector3f{ 0.f, 0.f, delta_depth });
	}

	void Window::translate_window(const Vector3f delta)
	{
		this->position += delta;

		this->group.translate(delta);
	}
}
}
