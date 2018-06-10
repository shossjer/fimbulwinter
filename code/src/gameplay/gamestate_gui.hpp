
#ifndef GAMEPLAY_GAMESTATE_GUI_HPP
#define GAMEPLAY_GAMESTATE_GUI_HPP

#include "gamestate_models.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/gui/gui.hpp"

#include <vector>

namespace gameplay
{
	namespace gamestate
	{
		engine::gui::data::KeyValue encode_gui(const Player &);

		engine::gui::data::KeyValue encode_gui(const std::vector<const Dish *> &);
	}
}

#endif /* GAMEPLAY_GAMESTATE_GUI_HPP */
