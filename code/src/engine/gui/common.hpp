
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
			utility::heap_storage<controller::list_t>,
			utility::heap_storage<controller::tab_t>
		>;

		using Interactions = core::container::MultiCollection
		<
			engine::Entity, 301,
			std::array<action::close_t, 51>,
			std::array<action::selection_t, 51>,
			std::array<action::tab_t, 51>
		>;

		using Reactions = node_map_t;

		// todo: the way in which others keep pointers to the views in
		// this collection is not okay, as the collection is free to
		// move the views around
		using Views = core::container::Collection
		<
			engine::Entity, 401,
			utility::static_storage<View, 201>
		>;
	}
}

#endif // ENGINE_GUI_COMMON_HPP
