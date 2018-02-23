
#include "writer.hpp"

#include "core/async/Thread.hpp"
#include "core/container/CircleQueue.hpp"
#include "core/debug.hpp"
#include "core/sync/Event.hpp"

#include <tuple>

namespace
{
	core::container::CircleQueue<std::tuple<int, engine::Entity, engine::Command, utility::any>, 100> queue_commands;

	core::async::Thread thread;
	volatile bool active = false;
	core::sync::Event<true> event;

	void process_messages()
	{
		std::tuple<int, engine::Entity, engine::Command, utility::any> command_args;
		while (queue_commands.try_pop(command_args))
		{
			debug_printline("record frame_count=", std::get<0>(command_args), " entity=", std::get<1>(command_args), " command=", std::get<2>(command_args), " data=", std::get<3>(command_args));
		}
	}

	void run()
	{
		event.wait();
		event.reset();

		while (active)
		{
			process_messages();

			event.wait();
			event.reset();
		}
	}
}

namespace engine
{
	namespace replay
	{
		void create()
		{
			active = true;
			event.reset();
			thread = core::async::Thread(run);
		}

		void destroy()
		{
			active = false;
			event.set();
			thread.join();
		}

		void post_add_command(int frame_count, engine::Entity entity, engine::Command command, utility::any && data)
		{
			auto res = queue_commands.try_emplace(frame_count, entity, command, std::move(data));
			debug_assert(res);
			if (res)
			{
				event.set();
			}
		}
	}
}
