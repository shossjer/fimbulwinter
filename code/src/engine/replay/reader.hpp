
#ifndef ENGINE_REPLAY_READER_HPP
#define ENGINE_REPLAY_READER_HPP

namespace engine
{
	namespace replay
	{
		void start(void * gamestate);

		void update(int frame_count);
	}
}

#endif /* ENGINE_REPLAY_READER_HPP */
