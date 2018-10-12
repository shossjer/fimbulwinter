
#include "writer.hpp"

#include "core/async/Thread.hpp"
#include "core/container/CircleQueue.hpp"
#include "core/debug.hpp"
#include "core/serialization.hpp"
#include "core/StringSerializer.hpp"
#include "core/sync/Event.hpp"

#include "engine/commands.hpp"

#include <atomic>
#include <fstream>
#include <tuple>

namespace
{
	core::container::CircleQueue<std::tuple<int, engine::Entity, engine::Command, utility::any>, 100> queue_commands;

	core::async::Thread thread;
	std::atomic_int active(0);
	core::sync::Event<true> event;

	template <typename Serializer>
	void process_messages(Serializer & s)
	{
		std::tuple<int, engine::Entity, engine::Command, utility::any> command_args;
		while (queue_commands.try_pop(command_args))
		{
			switch (std::get<2>(command_args))
			{
			case engine::command::RENDER_DESELECT:
			case engine::command::RENDER_HIGHLIGHT:
			case engine::command::RENDER_SELECT:
				debug_assert(std::get<3>(command_args).has_value());
				s.write(std::make_tuple(std::get<0>(command_args), std::get<1>(command_args), std::get<2>(command_args), utility::any_cast<engine::Entity>(std::get<3>(command_args))));
				break;
			default:
				debug_assert(!std::get<3>(command_args).has_value());
				s.write(std::make_tuple(std::get<0>(command_args), std::get<1>(command_args), std::get<2>(command_args)));
			}
		}
	}

	void run()
	{
		std::ofstream file("last.rec", std::ofstream::binary);
		core::StringSerializer serializer;
		// core::BinarySerializer serializer;

		event.wait();
		event.reset();

		while (active.load(std::memory_order_relaxed))
		{
			serializer.clear();
			process_messages(serializer);
			serializer.finalize();

			if (!serializer.empty())
			{
				file.write(serializer.data(), serializer.size());
			}

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
			active.store(1, std::memory_order_relaxed);
			event.reset();
			thread = core::async::Thread(run);
		}

		void destroy()
		{
			active.store(0, std::memory_order_relaxed);
			event.set();
			thread.join();
		}

		void post_add_command(int frame_count, engine::Entity entity, engine::Command command, utility::any && data)
		{
			if (active.load(std::memory_order_relaxed))
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
}