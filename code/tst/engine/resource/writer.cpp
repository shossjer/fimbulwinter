#include <catch.hpp>

#include "engine/resource/writer.hpp"

TEST_CASE("Resource writer can be created and destroyed", "[engine][resource]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::resource::writer writer;
	}
}
