#include "catch.hpp"

#include "core/async/delay.hpp"

#include "engine/audio/system.hpp"

TEST_CASE("Audio System can be created and destroyed", "[engine][audio]")
{
	engine::audio::System system;
}

TEST_CASE("Audio System can play sound", "[.engine][.audio]")
{
	engine::audio::System system;

	const int length = system.play();
	core::async::delay(length);
}
