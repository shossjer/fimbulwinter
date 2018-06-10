
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_COMMON_HPP
#define ENGINE_GUI_COMMON_HPP

#include "controller.hpp"
#include "interaction.hpp"
#include "reaction.hpp"
#include "view.hpp"

#include "core/container/Collection.hpp"

#include <array>
#include <vector>

namespace engine
{
	namespace gui
	{
		using Controllers = core::container::Collection
		<
			engine::Entity, 201,
			std::array<controller::list_t, 51>,
			std::array<controller::tab_t, 51>
		>;

		using Interactions = core::container::MultiCollection
		<
			engine::Entity, 301,
			std::array<action::close_t, 51>,
			std::array<action::selection_t, 51>,
			std::array<action::tab_t, 51>
		>;

		using Reactions = node_map_t;

		using Views = core::container::Collection
		<
			engine::Entity, 401,
			std::array<View, 201>
		>;
	}
}

#endif // ENGINE_GUI_COMMON_HPP
