
#ifndef GAMEPLAY_DEBUG_HPP
#define GAMEPLAY_DEBUG_HPP

#include <engine/debug.hpp>

namespace gameplay
{
	constexpr auto gameplay_channel = core::channel_t<0x0000000001000000ull>{};
}

#endif /* GAMEPLAY_DEBUG_HPP */
