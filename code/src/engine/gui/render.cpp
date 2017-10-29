
#include "render.hpp"

#include <engine/graphics/renderer.hpp>

namespace engine
{
	namespace gui
	{
		Matrix4x4f render_matrix(const View & view)
		{
			return make_translation_matrix(
				Vector3f{
					static_cast<float>(view.offset.width.value),
					static_cast<float>(view.offset.height.value),
					view.depth });
		}

		core::maths::Vector2f render_size(const View & view)
		{
			return core::maths::Vector2f{
				static_cast<float>(view.size.width.value),
				static_cast<float>(view.size.height.value) };
		}

		bool is_selectable(View & view)
		{
			return false;
		}

		void ViewRenderer::add(View & view, View::Color & content)
		{
			engine::graphics::renderer::post_add_panel(
				view.entity,
				engine::graphics::data::ui::PanelC{
					render_matrix(view),
					render_size(view),
					content.color != nullptr ? content.color->get(&view) : 0 });

			if (is_selectable(view))
			{
				engine::graphics::renderer::post_make_selectable(view.entity);
			}
		}
		void ViewRenderer::add(View & view, View::Text & content)
		{
			engine::graphics::renderer::post_add_text(
				view.entity,
				engine::graphics::data::ui::Text{
					render_matrix(view),
					content.color != nullptr ? content.color->get(&view) : 0,
					content.display});

			if (is_selectable(view))
			{
				engine::graphics::renderer::post_make_selectable(view.entity);
			}
		}

		void ViewRenderer::update(View & view, View::Color & content)
		{
			engine::graphics::renderer::post_update_panel(
				view.entity,
				engine::graphics::data::ui::PanelC{
					render_matrix(view),
					render_size(view),
					content.color != nullptr ? content.color->get(&view) : 0 });
		}
		void ViewRenderer::update(View & view, View::Text & content)
		{
			engine::graphics::renderer::post_update_text(
				view.entity,
				engine::graphics::data::ui::Text{
					render_matrix(view),
					content.color != nullptr ? content.color->get(&view) : 0,
					content.display});
		}

		void ViewRenderer::remove(View & view)
		{
			auto & status = view.status;

			if (status.rendered) // never mind if already removed...
			{
				engine::graphics::renderer::post_remove(view.entity);
				status.rendered = false;
			}
		}
	}
}
