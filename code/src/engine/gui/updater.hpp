
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_UPDATER_HPP
#define ENGINE_GUI_UPDATER_HPP

#include "views.hpp"

#include <engine/debug.hpp>

namespace engine
{
namespace gui
{
	struct ViewUpdater
	{
		static void list(List & view, const std::size_t size)
		{
			if (view.shown_items != size)
			{
				view.change.set_resized();
			}

			view.change.set_content();
			view.shown_items = size;
		}

		static void progress(ProgressBar & pb, const value_t progress)
		{
			if (pb.direction == ProgressBar::HORIZONTAL)
			{
				debug_assert(pb.target->size.width.type == Size::TYPE::PERCENT);
				pb.target->size.width.set_meta(progress);
			}
			else
			{
				debug_assert(pb.target->size.height.type == Size::TYPE::PERCENT);
				pb.target->size.height.set_meta(progress);
			}
			pb.target->change.set_resized();
		}

		// does not affect "visibility" of View
		// called when the whole Window is shown
		static void renderer_add(View & view)
		{
			if (view.should_render) return;	// already/marked for rendering
			if (view.is_invisible) return;	// if invisible it cannot be rendered

			for (auto child : view.get_children())
			{
				renderer_add(*child);
			}

			view.should_render = true;
			view.change.set_shown();
		}
		static void renderer_remove(View & view)
		{
			if (!view.should_render) return;

			for (auto child : view.get_children())
			{
				renderer_remove(*child);
			}

			view.should_render = false;
			view.change.set_hidden();
		}

		static void state(View & view, const Drawable::State data)
		{
			view.state = data;
			view.change.set_content();

			for (auto child : view.get_children())
			{
				state(*child, data);
			}
		}

		static void text(Text & view, const std::string data)
		{
			view.display = data;
			view.change.set_content();
		}

		static void texture(PanelT & view, const Asset data)
		{
			view.texture = data;
			view.change.set_content();
			view.change.set_resized();
		}

		static void translate(View & view, const Vector3f delta)
		{
			view.offset += delta;
			view.change.set_moved();

			for (auto child : view.get_children())
			{
				translate(*child, delta);
			}
		}

		static void visibility_hide(View & view)
		{
			if (view.is_invisible) return; // already invisible

			for (auto child : view.get_children())
			{
				visibility_hide(*child);
			}

			if (view.is_rendered)	// set as dirty if shown
			{
				view.change.set_hidden();
				view.change.set_resized();
			}
			else
			{
				view.change.clear();
			}

			view.is_invisible = true;
			view.should_render = false;
		}
		static void visibility_show(View & view)
		{
			if (!view.is_invisible) return; // already visible

			for (auto child : view.get_children())
			{
				visibility_show(*child);
			}

			view.is_invisible = false;
			view.change.set_shown();
			view.change.set_resized();
		}
	};
}
}

#endif // ENGINE_GUI_UPDATER_HPP
