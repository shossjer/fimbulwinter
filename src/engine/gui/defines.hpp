
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_DEFINES_HPP
#define ENGINE_GUI_DEFINES_HPP

#include "actions.hpp"
#include "views.hpp"

#include <core/container/Collection.hpp>

namespace engine
{
namespace gui
{
	typedef core::container::Collection
		<
			engine::Entity, 61,
			std::array<CloseAction, 10>,
			std::array<MoveAction, 10>,
			std::array<SelectAction, 10>
		>
		ACTIONS;

	typedef core::container::Collection
		<
			engine::Entity, 201,
			std::array<engine::gui::Group, 40>,
			std::array<engine::gui::PanelC, 50>,
			std::array<engine::gui::PanelT, 50>,
			std::array<engine::gui::Text, 50>
		>
		COMPONENTS;

	typedef core::container::Collection
		<
			engine::Asset, 21,
			std::array<engine::gui::Window, 10>,
			std::array<int, 10>
		>
		WINDOWS;
}
}

#endif // ENGINE_GUI_DEFINES_HPP
