
#include "gui.hpp"
#include "function.hpp"
#include "measure.hpp"
#include "update.hpp"
#include "view.hpp"

#include <core/container/Collection.hpp>

#include <vector>

using namespace engine::gui2;

namespace
{
	struct Window
	{
		View & view;

		// order etc
	};

	core::container::Collection
		<
		engine::Asset, 21,
		std::array<Window, 10>,
		std::array<int, 1>
		>
		windows;

	core::container::UnorderedCollection
		<
		engine::Entity, 100,
		std::array<Function, 100>,
		std::array<View, 100>
		>
		components;

	// TODO: update lookup structure
	// init from "gameplay" data structures
	// assign views during creation
}

namespace engine
{
	namespace gui2
	{

	}
}
