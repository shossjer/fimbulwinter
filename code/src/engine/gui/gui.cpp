
#include "creation.hpp"
#include "gui.hpp"
#include "function.hpp"
#include "loading.hpp"
#include "measure.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/CircleQueue.hpp>

#include <vector>

using namespace engine::gui;

namespace
{
	const uint32_t WINDOW_HEIGHT = 677; // 720
	const uint32_t WINDOW_WIDTH = 1004; // 1024

	struct Window
	{
		View * view;
		bool should_render = true;

		void hide()
		{
			should_render = false;
			ViewUpdater::hide(*view);
		}
		void show()
		{
			should_render = true;
			ViewUpdater::show(*view);
		}
	};

	core::container::Collection
		<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 1>
		>
		windows;

	Components components;

	// TODO: update lookup structure
	// init from "gameplay" data structures
	// assign views during creation

	using UpdateMessage = utility::variant
		<
		MessageInteraction,
		MessageVisibility
		>;

	core::container::CircleQueueSRMW<UpdateMessage, 100> queue_posts;
}

namespace engine
{
	namespace gui
	{
		extern std::vector<DataVariant> load();

		void create()
		{
			// load windows and views data from somewhere.
			std::vector<DataVariant> windows_data = load();

			for (auto & window_data : windows_data)
			{
				View & view = visit(Creator{ components }, window_data);

				windows.emplace<Window>("profile", &view);

				// TODO: use "window size" as size param
				ViewMeasure::refresh(view, Gravity::unmasked(), Offset{}, Size{ {Size::FIXED, height_t{ WINDOW_HEIGHT } }, { Size::FIXED, width_t{ WINDOW_WIDTH } } });
				ViewMeasure::refresh(view);
			}
		}

		void destroy()
		{
			// TODO: destroy something
		}

		void update()
		{
			struct
			{
				void operator () (MessageInteraction && x)
				{
					debug_printline(engine::gui_channel, "MessageInteraction");
				}
				void operator () (MessageVisibility && x)
				{
					debug_printline(engine::gui_channel, "MessageVisibility");

					if (!windows.contains(x.name))
					{
						debug_printline(engine::gui_channel, "Window visibility; window not found");
						return;
					}

					auto & window = windows.get<Window>(x.name);

					switch (x.state)
					{
					case MessageVisibility::HIDE:
						window.hide();
						break;
					case MessageVisibility::SHOW:
						window.show();
						break;
					case MessageVisibility::TOGGLE:
						if (window.should_render)
							window.hide();
						else
							window.show();
						break;
					}
				}
			} processMessage;

			UpdateMessage message;
			while (::queue_posts.try_pop(message))
			{
				visit(processMessage, std::move(message));
			}

			for (Window & window : windows.get<Window>())
			{
				if (window.should_render)
				{
					ViewMeasure::refresh(*window.view);
				}
			}
		}

		template<typename T>
		void _post(T && data)
		{
			const auto res = queue_posts.try_emplace(utility::in_place_type<T>, std::move(data));
			debug_assert(res);
		}

		template<typename T> void post(T && data) = delete;
		template<> void post(MessageInteraction && data) { _post(std::move(data)); }
		template<> void post(MessageVisibility && data) { _post(std::move(data)); }
	}
}
