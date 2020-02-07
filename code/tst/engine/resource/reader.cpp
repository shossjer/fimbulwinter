#include "engine/resource/reader.hpp"

#include <catch/catch.hpp>

TEST_CASE("Resource reader can be created and destroyed", "[engine][resource]")
{
	for (int i = 0; i < 2; i++)
	{
		engine::resource::reader reader;
	}
}
