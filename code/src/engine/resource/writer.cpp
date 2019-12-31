
#include "writer.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/debug.hpp"

#include "utility/string.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>

namespace
{
	bool file_exists(const std::string & filename)
	{
		std::ifstream stream(filename, std::ifstream::binary);
		return !!stream;
	}

	bool has_extension(const std::string & filename, const std::string & extension)
	{
		if (extension.size() > filename.size())
			return false;

		for (int i = 0; i < extension.size(); i++)
		{
			if (filename.data()[filename.size() - i - 1] != extension.data()[extension.size() - i - 1])
				return false;
		}
		return true;
	}
}

namespace
{
	struct MessageWrite
	{
		std::string name;
		void (* callback)(std::string name, engine::resource::writer::Serializer & serializer);
	};
	using Message = utility::variant
	<
		MessageWrite
	>;

	core::container::PageQueue<utility::heap_storage<Message>> queue_messages;
}

namespace
{
	void write_ini(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::writer::Serializer & serializer))
	{
		debug_printline("writing '", filename, "'");
		using SerializerType = core::IniSerializer;
		engine::resource::writer::Serializer serializer(utility::in_place_type<SerializerType>, filename);

		callback(std::move(name), serializer);

		const SerializerType & buffer = utility::get<SerializerType>(serializer);

		std::ofstream file(filename, std::ofstream::binary);
		debug_assert(file);

		file.write(buffer.data(), buffer.size());
	}

	void write_json(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::writer::Serializer & serializer))
	{
		debug_printline("writing '", filename, "'");
		using SerializerType = core::JsonSerializer;
		engine::resource::writer::Serializer serializer(utility::in_place_type<SerializerType>, filename);

		callback(std::move(name), serializer);

		std::string bytes = utility::get<SerializerType>(serializer).get();

		std::ofstream file(filename, std::ofstream::binary);
		debug_assert(file);

		file.write(bytes.data(), bytes.size());
	}

	void process_messages()
	{
		Message message;
		while (queue_messages.try_pop(message))
		{
			struct ProcessMessage
			{
				void operator () (MessageWrite && x)
				{
					if (has_extension(x.name, ".ini"))
					{
						write_ini(x.name, x.name, x.callback);
					}
					else if (has_extension(x.name, ".json"))
					{
						write_json(x.name, x.name, x.callback);
					}
					else
					{
						debug_fail("unknown file extension for '", x.name, "'");
					}
				}
			};
			visit(ProcessMessage{}, std::move(message));
		}
	}

	core::async::Thread renderThread;
	std::atomic_int active(0);
	core::sync::Event<true> event;

	void run()
	{
		event.wait();
		event.reset();

		while (active.load(std::memory_order_relaxed))
		{
			process_messages();

			event.wait();
			event.reset();
		}
	}
}

namespace engine
{
	namespace resource
	{
		writer::~writer()
		{
			debug_assert(active.load(std::memory_order_relaxed));

			active.store(0, std::memory_order_relaxed);
			event.set();

			renderThread.join();
		}

		writer::writer()
		{
			debug_assert(!active.load(std::memory_order_relaxed));

			active.store(1, std::memory_order_relaxed);
			renderThread = core::async::Thread{ run };
		}

		void writer::post_write(std::string name, void (* callback)(std::string name, Serializer & serializer))
		{
			const auto res = queue_messages.try_emplace(utility::in_place_type<MessageWrite>,
			                                            std::move(name),
			                                            callback);
			if (debug_verify(res, "write queue is full"))
			{
				event.set();
			}
		}
	}
}
