
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
				break;
			default:
				return;
			}

			auto & interaction = interactions.get<action::interaction_t>(data.entity);

			ViewUpdater::status(views.get<View>(interaction.target), state);
		}
	}
}
