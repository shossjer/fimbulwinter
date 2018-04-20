
// should not be included outside gui namespace.

#ifndef ENGINE_GUI_COMMON_HPP
#define ENGINE_GUI_COMMON_HPP

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
		using Interactions = core::container::Collection
		<
			engine::Entity, 51,
			std::array<Interaction, 101>
		>;

		using Reactions = std::unordered_map<engine::Asset, node_t>;

		using Views = core::container::Collection
		<
			engine::Entity, 201,
			std::array<View, 201>
		>;
	}
}

#endif // ENGINE_GUI_COMMON_HPP
