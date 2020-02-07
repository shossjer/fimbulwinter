
#include "writer.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/debug.hpp"

#include "utility/ranges.hpp"
#include "utility/string.hpp"
#include "utility/variant.hpp"

#include <atomic>
#include <fstream>

namespace
{
	bool has_extension(const std::string & filename, const std::string & extension)
	{
		if (extension.size() > filename.size())
			return false;

		for (std::ptrdiff_t i : ranges::index_sequence_for(extension))
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
	struct WriteData
	{
		std::ofstream file;

		WriteData(const char * filename)
			: file(filename, std::ofstream::binary)
		{}

		ext::ssize write(const void * dest, ext::usize n)
		{
			file.write(static_cast<const char *>(dest), n);

			return file ? n : ext::ssize(-1);
		}
	};

	ext::ssize write_callback(const void * src, ext::usize n, void * data)
	{
		WriteData & write_data = *static_cast<WriteData *>(data);

		return write_data.write(src, n);
	}

	template <typename SerializerType>
	void write_file(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::writer::Serializer & serializer))
	{
		debug_printline("writing '", filename, "'");
		WriteData write_data(filename.data());

		engine::resource::writer::Serializer serializer(utility::in_place_type<SerializerType>, core::WriteStream(write_callback, &write_data, std::move(filename)));

		callback(std::move(name), serializer);
	}

	void write_ini(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::writer::Serializer & serializer))
	{
		write_file<core::IniSerializer>(name, std::move(filename), callback);
	}

	void write_json(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::writer::Serializer & serializer))
	{
		write_file<core::JsonSerializer>(name, std::move(filename), callback);
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
						write_ini(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
					}
					else if (has_extension(x.name, ".json"))
					{
						write_json(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
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
