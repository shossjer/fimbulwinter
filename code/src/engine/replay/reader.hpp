
#ifndef ENGINE_REPLAY_READER_HPP
#define ENGINE_REPLAY_READER_HPP

#include "engine/Command.hpp"
#include "engine/Entity.hpp"

#include "utility/any.hpp"

namespace engine
{
	namespace replay
	{
		void start(void (* callback_command)(engine::Entity entity, engine::Command command, utility::any && data));

		void update(int frame_count);
	}
}

#endif /* ENGINE_REPLAY_READER_HPP */
