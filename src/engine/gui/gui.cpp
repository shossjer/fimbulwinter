
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

	void update(engine::Asset window, engine::gui::Datas && datas)
	{
		// TODO: use thread safe queue
	}

	void update(engine::Asset window, core::maths::Vector3f delta)
	{
		// TODO: use thread safe queue
		windows.get<Window>(window).translate_window(delta);
	}
}
}
