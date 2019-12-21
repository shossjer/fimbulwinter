
#ifndef ENGINE_REPLAY_WRITER_HPP
#define ENGINE_REPLAY_WRITER_HPP

#include "engine/Command.hpp"
#include "engine/Entity.hpp"

#include "utility/any.hpp"

namespace engine
{
	class record
	{
	public:
		~record();
		record();
	};

	void post_add_command(record & record, int frame_count, engine::Entity entity, engine::Command command, utility::any && data);
}

#endif /* ENGINE_REPLAY_WRITER_HPP */
