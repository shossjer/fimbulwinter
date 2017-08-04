
#include "defines.hpp"
#include "gui.hpp"

#include <utility/json.hpp>

#include <fstream>

namespace
{
	engine::gui::ACTIONS actions;

	engine::gui::COMPONENTS components;

	engine::gui::WINDOWS windows;

	std::vector<engine::gui::Window *> window_stack;
}

namespace engine
{
namespace gui
{
	extern void load(ACTIONS &, COMPONENTS &, WINDOWS & windows);

	// lookup table for "named" components (asset -> entity)
	// used when gamestate requests updates of components.
	core::container::Collection
		<
		engine::Asset, 201,
		std::array<engine::Entity, 100>,
		std::array<int, 1>
		>
		lookup;

	void create()
	{
		load(actions, components, windows);

		for (Window & window : windows.get<Window>())
		{
			window_stack.push_back(&window);
		}
	}

	void destroy()
	{
		// TODO: destroy something
	}

	void show(engine::Asset window)
	{
		select(window);
		windows.get<Window>(window).show_window();
	}

	void hide(engine::Asset window)
	{
		windows.get<Window>(window).hide_window();
	}

	void toggle(engine::Asset window)
	{
		auto & w = windows.get<Window>(window);

		if (w.is_shown()) w.hide_window();
		else w.show_window();
	}

	struct Trigger
	{
		void operator() (const CloseAction & action)
		{
			hide(action.window);
		}

		template<typename T>
		void operator() (const T &)
		{}
	};

	void select(engine::Asset window)
	{
		for (std::size_t i = 0; i < window_stack.size(); i++)
		{
			if (window_stack[i]->name == window)
			{
				window_stack.erase(window_stack.begin() + i);
				window_stack.insert(window_stack.begin(), &windows.get<Window>(window));
				break;
			}
		}

		for (std::size_t i = 0; i < window_stack.size(); i++)
		{
			const int order = static_cast<int>(i);
			window_stack[i]->reorder_window(-order * 10);
		}
	}

	void trigger(engine::Entity entity)
	{
		debug_assert(actions.contains(entity));

		// TODO: use thread safe queue
		actions.call(entity, Trigger{});
	}

	struct Updater
	{
		Data data;

		void operator() (PanelC & panel)
		{
			switch (this->data.type)
			{
			case Data::COLOR:

				panel.color = data.color;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for color panel.");
				debug_unreachable();
			}
		}

		void operator() (PanelT & panel)
		{
			switch (this->data.type)
			{
			case Data::TEXTURE:

				panel.texture = data.texture;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for texture panel.");
				debug_unreachable();
			}
		}

		void operator() (Text & text)
		{
			switch (this->data.type)
			{
			case Data::COLOR:

				text.color = data.color;
				break;

			case Data::DISPLAY:

				text.display = data.display;
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for text.");
				debug_unreachable();
			}
		}

		void operator() (ProgressBar & pb)
		{
			switch (this->data.type)
			{
			case Data::PROGRESS:

				if (pb.direction == ProgressBar::HORIZONTAL)
				{
					debug_assert(pb.target->size.width.type == View::Size::TYPE::PERCENTAGE);
					pb.target->size.width.set_meta(this->data.progress);
				}
				else
				{
					debug_assert(pb.target->size.height.type == View::Size::TYPE::PERCENTAGE);
					pb.target->size.height.set_meta(this->data.progress);
				}
				break;

			default:

				debug_printline(0xffffffff, "GUI - invalid update type for progress bar.");
				debug_unreachable();
			}
		}

		template<typename T>
		void operator() (const T &)
		{
			debug_printline(0xffffffff, "GUI - update of unknown component type.");
		}
	};

	void update(engine::Asset window, engine::gui::Datas && datas)
	{
		// TODO: use thread safe queue

		for (auto & data : datas)
		{
			if (!lookup.contains(data.first))
			{
				debug_printline(0xffffffff, "GUI - cannot find named component for update.");
				continue;
			}

			engine::Entity entity = lookup.get<engine::Entity>(data.first);

			components.call(entity, Updater{ std::move(data.second) });
		}

		// updates and sends updated views to renderer (if shown)
		windows.get<Window>(window).update_window();
	}

	void update(engine::Asset window, core::maths::Vector3f delta)
	{
		// TODO: use thread safe queue
		windows.get<Window>(window).translate_window(delta);
	}
}
}
