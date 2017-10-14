
#ifndef GAMEPLAY_DEBUG_HPP
#define GAMEPLAY_DEBUG_HPP

#include <engine/debug.hpp>

#ifdef __GNUG__
# define gameplay_debug_print(...)     debug_print(gameplay::gameplay_channel, ##__VA_ARGS__)
# define gameplay_debug_printline(...) debug_printline(gameplay::gameplay_channel, ##__VA_ARGS__)
#else
# define gameplay_debug_print(...)     debug_print(gameplay::gameplay_channel, __VA_ARGS__)
# define gameplay_debug_printline(...) debug_printline(gameplay::gameplay_channel, __VA_ARGS__)
#endif

namespace gameplay
{
	enum channel_t : unsigned
	{
		gameplay_channel      = 1 << (engine::n_channels + 0),
		n_channels          = engine::n_channels + 1
	};
}

#endif /* GAMEPLAY_DEBUG_HPP */
