
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

constexpr engine::Asset engine::gui::ViewData::Action::CLOSE;
constexpr engine::Asset engine::gui::ViewData::Action::INTERACTION;
constexpr engine::Asset engine::gui::ViewData::Action::MOVER;
constexpr engine::Asset engine::gui::ViewData::Action::SELECT;
constexpr engine::Asset engine::gui::ViewData::Action::TRIGGER;

namespace
{
	const uint32_t WINDOW_HEIGHT = 677; // 720
	const uint32_t WINDOW_WIDTH = 1004; // 1024

	Actions actions;

	Components components;

	View * screen_view = nullptr;
	View::Group * screen_group = nullptr;

	// TODO: update lookup structure
	// init from "gameplay" data structures
	// assign views during creation
	core::container::Collection<engine::Asset, 11, std::array<View*, 10>> lookup;

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

			{	// create the "screen" view of the screen
				// TODO: use "window size" as size param
				const auto entity = engine::Entity::create();
				screen_view = &components.emplace<View>(
					entity,
					entity,
					View::Group{ View::Group::Layout::RELATIVE },
					Gravity{},
					Margin{},
					Size{ {Size::FIXED, height_t{ WINDOW_HEIGHT }, }, {Size::FIXED, width_t{ WINDOW_WIDTH } } },
					nullptr);
			}

			screen_group = &utility::get<View::Group>(screen_view->content);

			for (auto & window_data : windows_data)
			{
				View & view = visit(Creator{ actions, components }, window_data);

				// temp
				lookup.emplace<View*>(Asset{"profile"}, &view);

				screen_group->adopt(&view);
				view.parent = screen_view;
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

					struct Updater
					{
						MessageInteraction & message;

						void operator() (const CloseAction & action)
						{
							if (message.interaction == MessageInteraction::CLICK)
							{
								ViewUpdater::hide(components.get<View>(action.target));
							}
						}
						void operator() (const InteractionAction & action)
						{
							Status::State state;
							switch (message.interaction)
							{
							case MessageInteraction::HIGHLIGHT:
								state = Status::HIGHLIGHT;
								break;
							case MessageInteraction::LOWLIGHT:
								state = Status::DEFAULT;
								break;
							case MessageInteraction::PRESS:
								state = Status::PRESSED;
								break;
							case MessageInteraction::RELEASE:
								state = Status::HIGHLIGHT;
								break;
							default:
								return;
							}

							ViewUpdater::status(
								components.get<View>(action.target),
								state);
						}
					};
					actions.call_all(x.entity, Updater{ x });
				}
				void operator () (MessageVisibility && x)
				{
					debug_printline(engine::gui_channel, "MessageVisibility");

					if (!lookup.contains(x.window))
						return;

					View & view = *lookup.get<View*>(x.window);

					switch (x.state)
					{
					case MessageVisibility::HIDE:
						ViewUpdater::hide(view);
						break;
					case MessageVisibility::SHOW:
						ViewUpdater::show(view);
						break;
					case MessageVisibility::TOGGLE:
						if (view.status.should_render)
							ViewUpdater::hide(view);
						else
							ViewUpdater::show(view);
						break;
					}
				}
			} processMessage;

			UpdateMessage message;
			while (::queue_posts.try_pop(message))
			{
				visit(processMessage, std::move(message));
			}

			ViewMeasure::refresh(*screen_view);
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
