
#ifndef ENGINE_DEBUG_HPP
#define ENGINE_DEBUG_HPP

#include <core/debug.hpp>

#ifdef __GNUG__
# define engine_debug_print(...)      debug_print(engine::engine_channel, ##__VA_ARGS__)
# define animation_debug_print(...)   debug_print(engine::animation_channel, ##__VA_ARGS__)
# define application_debug_print(...) debug_print(engine::application_channel, ##__VA_ARGS__)
# define audio_debug_print(...)       debug_print(engine::audio_channel, ##__VA_ARGS__)
# define graphics_debug_print(...)    debug_print(engine::graphics_channel, ##__VA_ARGS__)
# define physics_debug_print(...)     debug_print(engine::physics_channel, ##__VA_ARGS__)

# define engine_debug_printline(...)      debug_printline(engine::engine_channel, ##__VA_ARGS__)
# define animation_debug_printline(...)   debug_printline(engine::animation_channel, ##__VA_ARGS__)
# define application_debug_printline(...) debug_printline(engine::application_channel, ##__VA_ARGS__)
# define audio_debug_printline(...)       debug_printline(engine::audio_channel, ##__VA_ARGS__)
# define graphics_debug_printline(...)    debug_printline(engine::graphics_channel, ##__VA_ARGS__)
# define physics_debug_printline(...)     debug_printline(engine::physics_channel, ##__VA_ARGS__)

# define engine_debug_trace(...)      debug_trace(engine::engine_channel, ##__VA_ARGS__)
# define animation_debug_trace(...)   debug_trace(engine::animation_channel, ##__VA_ARGS__)
# define application_debug_trace(...) debug_trace(engine::application_channel, ##__VA_ARGS__)
# define audio_debug_trace(...)       debug_trace(engine::audio_channel, ##__VA_ARGS__)
# define graphics_debug_trace(...)    debug_trace(engine::graphics_channel, ##__VA_ARGS__)
# define physics_debug_trace(...)     debug_trace(engine::physics_channel, ##__VA_ARGS__)
#else
# define engine_debug_print(...)      debug_print(engine::engine_channel, __VA_ARGS__)
# define animation_debug_print(...)   debug_print(engine::animation_channel, __VA_ARGS__)
# define application_debug_print(...) debug_print(engine::application_channel, __VA_ARGS__)
# define audio_debug_print(...)       debug_print(engine::audio_channel, __VA_ARGS__)
# define graphics_debug_print(...)    debug_print(engine::graphics_channel, __VA_ARGS__)
# define physics_debug_print(...)     debug_print(engine::physics_channel, __VA_ARGS__)

# define engine_debug_printline(...)      debug_printline(engine::engine_channel, __VA_ARGS__)
# define animation_debug_printline(...)   debug_printline(engine::animation_channel, __VA_ARGS__)
# define application_debug_printline(...) debug_printline(engine::application_channel, __VA_ARGS__)
# define audio_debug_printline(...)       debug_printline(engine::audio_channel, __VA_ARGS__)
# define graphics_debug_printline(...)    debug_printline(engine::graphics_channel, __VA_ARGS__)
# define physics_debug_printline(...)     debug_printline(engine::physics_channel, __VA_ARGS__)

# define engine_debug_trace(...)      debug_trace(engine::engine_channel, __VA_ARGS__)
# define animation_debug_trace(...)   debug_trace(engine::animation_channel, __VA_ARGS__)
# define application_debug_trace(...) debug_trace(engine::application_channel, __VA_ARGS__)
# define audio_debug_trace(...)       debug_trace(engine::audio_channel, __VA_ARGS__)
# define graphics_debug_trace(...)    debug_trace(engine::graphics_channel, __VA_ARGS__)
# define physics_debug_trace(...)     debug_trace(engine::physics_channel, __VA_ARGS__)
#endif

namespace engine
{
	enum channel_t : unsigned
	{
		engine_channel      = 1 << (core::n_channels + 0),
		animation_channel   = 1 << (core::n_channels + 1),
		application_channel = 1 << (core::n_channels + 2),
		audio_channel       = 1 << (core::n_channels + 3),
		graphics_channel    = 1 << (core::n_channels + 4),
		physics_channel     = 1 << (core::n_channels + 5),
		n_channels          = core::n_channels + 6
	};
}

#endif /* ENGINE_DEBUG_HPP */
