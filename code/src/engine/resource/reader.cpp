
#include "reader.hpp"

#include "core/async/Thread.hpp"
#include "core/container/Queue.hpp"
#include "core/sync/Event.hpp"

#include "engine/debug.hpp"

#include "utility/ranges.hpp"
#include "utility/string.hpp"
#include "utility/variant.hpp"

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

		for (std::ptrdiff_t i : ranges::index_sequence_for(extension))
		{
			if (filename.data()[filename.size() - i - 1] != extension.data()[extension.size() - i - 1])
				return false;
		}
		return true;
	}

	bool check_if_arm(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".arm"));
	}
	bool check_if_glsl(const std::string & name)
	{
		return file_exists(utility::to_string("res/gfx/", name, ".glsl"));
	}
	int check_if_ini(const std::string & name)
	{
		return
			file_exists(name + ".ini") ? 1 :
			file_exists("res/" + name + ".ini") ? 2 :
			0;
	}
	int check_if_json(const std::string & name)
	{
		return
			file_exists(name + ".json") ? 1 :
			file_exists("res/" + name + ".json") ? 2 :
			0;
	}
	bool check_if_lvl(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".lvl"));
	}
	bool check_if_msh(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".msh"));
	}
	bool check_if_png(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".png"));
	}
	bool check_if_ttf(const std::string & name)
	{
		return file_exists(utility::to_string("res/font/", name, ".ttf"));
	}
}

namespace
{
	struct MessageRead
	{
		std::string name;
		void (* callback)(std::string name, engine::resource::reader::Structurer && structurer);
		engine::resource::FormatMask formats;
	};
	using Message = utility::variant
	<
		MessageRead
	>;

	core::container::PageQueue<utility::heap_storage<Message>> queue_messages;
}

namespace
{
	struct ReadData
	{
		std::ifstream file;

		ReadData(const char * filename)
			: file(filename, std::ifstream::binary)
		{
			if (!file)
				throw std::runtime_error("");
		}

		int64_t read(char * dest, int64_t n)
		{
			file.read(dest, n);

			return file.gcount();
		}
	};

	uint64_t read_callback(char * dest, int64_t n, void * data)
	{
		debug_assert(n < 0x7fffffffffffffffll);
		ReadData & read_data = *static_cast<ReadData *>(data);

		uint64_t amount = read_data.read(dest, n);
		if (int64_t(amount) < n)
			amount |= 0x8000000000000000ll;

		return amount;
	}

	template <typename StructurerType>
	void read_file(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		ReadData read_data(filename.data());

		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, core::ReadStream(read_callback, &read_data, std::move(filename)));

		callback(std::move(name), std::move(structurer));
	}

	void read_arm(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::ArmatureStructurer>(name, std::move(filename), callback);
	}

	void read_bytes(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::BytesStructurer>(name, std::move(filename), callback);
	}

	void read_glsl(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::ShaderStructurer>(name, std::move(filename), callback);
	}

	void read_ini(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::IniStructurer>(name, std::move(filename), callback);
	}

	void read_json(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::JsonStructurer>(name, std::move(filename), callback);
	}

	void read_lvl(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::LevelStructurer>(name, std::move(filename), callback);
	}

	void read_msh(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::PlaceholderStructurer>(name, std::move(filename), callback);
	}

	void read_png(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		read_file<core::PngStructurer>(name, std::move(filename), callback);
	}

	void no_read(std::string name, utility::heap_string_utf8 && filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		using StructurerType = core::NoSerializer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, std::move(filename));
		callback(std::move(name), std::move(structurer));
	}

	void process_messages()
	{
		Message message;
		while (queue_messages.try_pop(message))
		{
			struct ProcessMessage
			{
				void operator () (MessageRead && x)
				{
					if (file_exists(x.name))
					{
						if ((x.formats & engine::resource::Format::Armature) && has_extension(x.name, ".arm"))
						{
							read_arm(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Shader) && has_extension(x.name, ".glsl"))
						{
							read_glsl(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Ini) && has_extension(x.name, ".ini"))
						{
							read_ini(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Json) && has_extension(x.name, ".json"))
						{
							read_json(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Level) && has_extension(x.name, ".lvl"))
						{
							read_lvl(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Placeholder) && has_extension(x.name, ".msh"))
						{
							read_msh(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Png) && has_extension(x.name, ".png"))
						{
							read_png(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if ((x.formats & engine::resource::Format::Ttf) && has_extension(x.name, ".ttf"))
						{
							read_bytes(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else if (x.formats & engine::resource::FormatMask::all())
						{
							debug_fail("unknown file format for '", x.name, "'");
						}
						else
						{
							debug_fail("format mask forbids reading of '", x.name, "'");
						}
					}
					else
					{
						const engine::resource::FormatMask available_formats =
							(engine::resource::FormatMask::fill(check_if_arm(x.name)) & engine::resource::Format::Armature) |
							(engine::resource::FormatMask::fill(check_if_ini(x.name)) & engine::resource::Format::Ini) |
							(engine::resource::FormatMask::fill(check_if_json(x.name)) & engine::resource::Format::Json) |
							(engine::resource::FormatMask::fill(check_if_lvl(x.name)) & engine::resource::Format::Level) |
							(engine::resource::FormatMask::fill(check_if_msh(x.name)) & engine::resource::Format::Placeholder) |
							(engine::resource::FormatMask::fill(check_if_png(x.name)) & engine::resource::Format::Png) |
							(engine::resource::FormatMask::fill(check_if_glsl(x.name)) & engine::resource::Format::Shader) |
							(engine::resource::FormatMask::fill(check_if_ttf(x.name)) & engine::resource::Format::Ttf);

						const engine::resource::FormatMask matching_formats = available_formats & x.formats;
						if (!matching_formats.unique())
						{
							debug_fail("more than one format matches '", x.name, "'");
						}
						else if (matching_formats & engine::resource::Format::Armature)
						{
							utility::heap_string_utf8 filename = u8"res/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".arm");

							read_arm(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Ini)
						{
							utility::heap_string_utf8 filename = check_if_ini(x.name) == 2 ? u8"res/" : u8""; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".ini");

							read_ini(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Json)
						{
							utility::heap_string_utf8 filename = check_if_json(x.name) == 2 ? u8"res/" : u8""; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".json");

							read_json(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Level)
						{
							utility::heap_string_utf8 filename = u8"res/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".lvl");

							read_lvl(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Placeholder)
						{
							utility::heap_string_utf8 filename = u8"res/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".msh");

							read_msh(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Png)
						{
							utility::heap_string_utf8 filename = u8"res/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".png");

							read_png(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats & engine::resource::Format::Shader)
						{
							utility::heap_string_utf8 filename = u8"res/gfx/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".glsl");

							read_glsl(x.name, std::move(filename), x.callback);
						}
						else if (matching_formats)
						{
							debug_fail("unknown file format for '", x.name, "'");
						}
						else if (matching_formats & engine::resource::Format::Ttf)
						{
							utility::heap_string_utf8 filename = u8"res/font/"; // todo
							filename.try_append(x.name.data(), utility::unit_difference(x.name.size()));
							filename.try_append(u8".ttf");

							read_bytes(x.name, std::move(filename), x.callback);
						}
						else if (x.formats & engine::resource::Format::None)
						{
							no_read(x.name, utility::heap_string_utf8(x.name.data(), utility::unit_difference(x.name.size())), x.callback);
						}
						else
						{
							debug_fail("format mask forbids reading of '", x.name, "'");
						}
					}
				}
			};
			visit(ProcessMessage{}, std::move(message));
		}
	}

	std::atomic_int active(0);

	core::async::Thread readerThread;
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
		reader::~reader()
		{
			debug_assert(active.load(std::memory_order_relaxed));

			active.store(0, std::memory_order_relaxed);
			event.set();

			readerThread.join();
		}

		reader::reader()
		{
			debug_assert(!active.load(std::memory_order_relaxed));

			active.store(1, std::memory_order_relaxed);
			readerThread = core::async::Thread{ run };
		}

		void reader::post_read(std::string name,
		                       void (* callback)(std::string name, Structurer && structurer),
		                       FormatMask formats)
		{
			const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRead>,
			                                            std::move(name),
			                                            callback,
			                                            formats);
			if (debug_verify(res, "read queue is full"))
			{
				event.set();
			}
		}
	}
}
