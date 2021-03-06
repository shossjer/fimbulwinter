
#ifndef ENGINE_COMMANDS_HPP
#define ENGINE_COMMANDS_HPP

#include "engine/Command.hpp"

#define DEFINE_COMMAND(name) constexpr engine::Command name = engine::Command(#name)

namespace engine
{
	namespace command
	{
		DEFINE_COMMAND(LOADER_ASSET_COUNT);
		DEFINE_COMMAND(LOADER_FINISHED);
		DEFINE_COMMAND(LOADER_READ_BEGIN);
		DEFINE_COMMAND(LOADER_READ_END);
	}
}

#endif /* ENGINE_COMMANDS_HPP */
