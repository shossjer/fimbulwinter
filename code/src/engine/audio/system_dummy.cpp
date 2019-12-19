#include "config.h"

#if AUDIO_USE_DUMMY

#include "system.hpp"

#include "core/debug.hpp"

namespace
{
	std::atomic_bool created(false);
}

namespace engine
{
	namespace audio
	{
		System::~System()
		{
			debug_assert(created.load(std::memory_order_relaxed));

			created.store(false, std::memory_order_release);
		}

		System::System()
		{
			debug_assert(!created.load(std::memory_order_relaxed));

			created.store(true, std::memory_order_release);
		}

		int System::play()
		{
			debug_assert(created.load(std::memory_order_relaxed));

			return 0;
		}
	}
}

#endif
