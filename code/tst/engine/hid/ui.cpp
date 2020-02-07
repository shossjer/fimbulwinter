#include "engine/hid/ui.hpp"

#include <catch/catch.hpp>

TEST_CASE("Human input device User input can be created and destroyed", "[engine][hid]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::hid::ui ui;
	}
}
