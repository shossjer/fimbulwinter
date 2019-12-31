#include <catch.hpp>

#include "engine/replay/writer.hpp"

TEST_CASE("Record can be created and destroyed", "[engine][replay]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::record record;
	}
}
