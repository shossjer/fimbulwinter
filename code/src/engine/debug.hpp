
#ifndef ENGINE_DEBUG_HPP
#define ENGINE_DEBUG_HPP

#include <core/debug.hpp>

namespace engine
{
	constexpr auto engine_channel = core::channel_t<0x0000000000000100ull>{};
	constexpr auto animation_channel = core::channel_t<0x0000000000000300ull>{};
	constexpr auto application_channel = core::channel_t<0x0000000000000500ull>{};
	constexpr auto audio_channel = core::channel_t<0x0000000000000900ull>{};
	constexpr auto graphics_channel = core::channel_t<0x0000000000001100ull>{};
	constexpr auto gui_channel = core::channel_t<0x0000000000002100ull>{};
	constexpr auto physics_channel = core::channel_t<0x0000000000004100ull>{};
	constexpr auto resource_channel = core::channel_t<0x0000000000008100ull>{};
}

#endif /* ENGINE_DEBUG_HPP */
