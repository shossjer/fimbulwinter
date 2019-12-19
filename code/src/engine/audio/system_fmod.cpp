#include "config.h"

#if AUDIO_USE_FMOD

#include "system.hpp"

#include "core/debug.hpp"

#include "utility/storing.hpp"

#include <atomic>

#include <fmod.hpp>
#include <fmod_errors.h>

namespace
{
	struct Data
	{
		FMOD::System * system;
		FMOD::Sound * sound;
	};

	std::atomic_bool has_data(false);
	utility::storing<Data> data;
}

namespace engine
{
	namespace audio
	{
		System::~System()
		{
			if (debug_assert(has_data.load(std::memory_order_acquire)))
			{
				FMOD::System * system = data.value.system;
				FMOD::Sound * sound = data.value.sound;

				has_data.store(false, std::memory_order_relaxed);
				data.destruct();

				sound->release(); // ignore result
				system->close(); // ignore result
				system->release(); // ignore result
			}
		}

		System::System()
		{
			debug_assert(!has_data.load(std::memory_order_relaxed));

			FMOD::System * system;
			if (FMOD_RESULT result = FMOD::System_Create(&system))
			{
				debug_fail("FMOD::System_Create(&system) failed with ", FMOD_ErrorString(result), "\b (", result, ")");
				return;
			}

			unsigned int version;
			if (FMOD_RESULT result = system->getVersion(&version))
			{
				system->release(); // ignore result
				debug_fail("system->getVersion(&version) failed with ", FMOD_ErrorString(result), "\b (", result, ")");
				return;
			}
			if (version < FMOD_VERSION)
			{
				system->release(); // ignore result
				debug_fail("FMOD lib version ", version, " does not match header version ", FMOD_VERSION);
				return;
			}

			if (FMOD_RESULT result = system->init(32, FMOD_INIT_NORMAL, nullptr))
			{
				system->release(); // ignore result
				debug_fail("system->init(32, FMOD_INIT_NORMAL, nullptr) failed with ", FMOD_ErrorString(result), "\b (", result, ")");
				return;
			}

			FMOD::Sound * sound;
			if (FMOD_RESULT result = system->createSound("res/sfx/jaguar.wav", FMOD_DEFAULT, nullptr, &sound))
			{
				system->close(); // ignore result
				system->release(); // ignore result
				debug_fail("system->createSound(\"res/sfx/jaguar.wav\", FMOD_DEFAULT, nullptr, &sound) failed with ", FMOD_ErrorString(result), "\b (", result, ")");
				return;
			}

			data.construct(system, sound);
			has_data.store(true, std::memory_order_release);
		}

		int System::play()
		{
			debug_assert(has_data.load(std::memory_order_acquire));
			FMOD::Channel * channel;
			data.value.system->playSound(data.value.sound, nullptr, false, &channel);

			unsigned int length;
			data.value.sound->getLength(&length, FMOD_TIMEUNIT_MS);
			return length;
		}
	}
}

#endif
