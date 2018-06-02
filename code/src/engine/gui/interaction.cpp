
#include "interaction.hpp"

#include "common.hpp"
#include "update.hpp"

namespace
{

}

namespace engine
{
	namespace gui
	{
		void update(const Resources & resources, MessageInteraction & data, Interactions & interactions, Views & views)
		{
			debug_printline(engine::gui_channel, "interaction received: ", data.entity);

			Status::State state;
			bool clicked = false;
			switch (data.interaction)
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
				clicked = true;
				break;
			default:
				return;
			}

			struct
			{
				Views & views;
				Status::State state;
				bool clicked;

				void operator() (action::close_t & a)
				{
					if (clicked)
					{
						auto & view = views.get<View>(a.target);
						ViewUpdater::hide(view);
					}
				}

				void operator() (action::interaction_t & a)
				{
					auto & view = views.get<View>(a.target);
					ViewUpdater::status(view, state);
				}

			}
			lookup{ views, state, clicked };

			interactions.call_all(data.entity, lookup);
		}
	}
}
