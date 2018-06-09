
#include "interaction.hpp"

#include "common.hpp"
#include "controller.hpp"
#include "update.hpp"

namespace engine
{
	namespace gui
	{
		extern Controllers controllers;
		extern Interactions interactions;
		extern Reactions reactions;
		extern Views views;

		namespace controller
		{
			extern void activate(Controllers & controllers, Views & views, action::tab_t & action);
		}
	}
}

namespace
{
	using namespace engine::gui;

	auto get_state(const MessageInteraction & data)
	{
		switch (data.interaction)
		{
		case MessageInteraction::HIGHLIGHT:
			return Status::HIGHLIGHT;
		case MessageInteraction::LOWLIGHT:
			return Status::DEFAULT;
		case MessageInteraction::PRESS:
			return Status::PRESSED;
		case MessageInteraction::RELEASE:
			return Status::HIGHLIGHT;
		default:
			return Status::DEFAULT;
		}
	}

	auto is_click(const MessageInteraction & data)
	{
		return data.interaction == MessageInteraction::RELEASE;
	}
}

namespace engine
{
	namespace gui
	{
		void update(const Resources & resources, MessageInteraction & data)
		{
			debug_printline(engine::gui_channel, "interaction received: ", data.entity);

			struct
			{
				const MessageInteraction & data;
				Views & views;

				void operator() (action::close_t & a)
				{
					if (is_click(data))
					{
						auto & view = views.get<View>(a.target);
						ViewUpdater::hide(view);
					}
				}

				void operator() (action::selection_t & a)
				{
					auto & view = views.get<View>(a.target);
					ViewUpdater::status(view, get_state(data));
				}

				void operator() (action::tab_t & a)
				{
					if (is_click(data))
					{
						// notify controller about tab click
						controller::activate(controllers, views, a);
					}
				}

			}
			lookup{ data, views};

			interactions.call_all(data.entity, lookup);
		}
	}
}
