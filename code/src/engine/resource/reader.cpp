
#include "reader.hpp"

#include <core/async/Thread.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/sync/Event.hpp>

#include <engine/debug.hpp>

#include <utility/string.hpp>
#include <utility/variant.hpp>

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

	core::container::CircleQueueSRMW<Message, 100> queue_messages;
}

namespace
{
	void read_arm(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		using StructurerType = core::ArmatureStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, file_size, filename);
		file.read(utility::get<StructurerType>(structurer).data(), file_size);

		callback(std::move(name), std::move(structurer));
	}

	void read_glsl(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		using StructurerType = core::ShaderStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, file_size, filename);
		file.read(utility::get<StructurerType>(structurer).data(), file_size);

		callback(std::move(name), std::move(structurer));
	}

	void read_ini(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		core::SmallFile stream(file_size, filename);
		file.read(stream.data(), file_size);

		using StructurerType = core::IniStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, stream);

		callback(std::move(name), std::move(structurer));
	}

	void read_json(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		std::vector<char> bytes(file_size);
		file.read(bytes.data(), file_size);

		using StructurerType = core::JsonStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, filename);
		utility::get<StructurerType>(structurer).set(bytes.data(), bytes.size());

		callback(std::move(name), std::move(structurer));
	}

	void read_lvl(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		using StructurerType = core::LevelStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, file_size, filename);
		file.read(utility::get<StructurerType>(structurer).data(), file_size);

		callback(std::move(name), std::move(structurer));
	}

	void read_msh(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		using StructurerType = core::PlaceholderStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, file_size, filename);
		file.read(utility::get<StructurerType>(structurer).data(), file_size);

		callback(std::move(name), std::move(structurer));
	}

	void read_png(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		debug_printline("reading '", filename, "'");
		std::ifstream file(filename, std::ifstream::binary);
		debug_assert(file);

		file.seekg(0, std::ifstream::end);
		const auto file_size = file.tellg();
		file.seekg(0, std::ifstream::beg);

		using StructurerType = core::PngStructurer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, file_size, filename);
		file.read(utility::get<StructurerType>(structurer).data(), file_size);

		callback(std::move(name), std::move(structurer));
	}

	void no_read(std::string name, std::string filename, void (* callback)(std::string name, engine::resource::reader::Structurer && structurer))
	{
		using StructurerType = core::NoSerializer;
		engine::resource::reader::Structurer structurer(utility::in_place_type<StructurerType>, filename);
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
							read_arm(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Shader) && has_extension(x.name, ".glsl"))
						{
							read_glsl(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Ini) && has_extension(x.name, ".ini"))
						{
							read_ini(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Json) && has_extension(x.name, ".json"))
						{
							read_json(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Level) && has_extension(x.name, ".lvl"))
						{
							read_lvl(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Placeholder) && has_extension(x.name, ".msh"))
						{
							read_msh(x.name, x.name, x.callback);
						}
						else if ((x.formats & engine::resource::Format::Png) && has_extension(x.name, ".png"))
						{
							read_png(x.name, x.name, x.callback);
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
							(engine::resource::FormatMask::fill(check_if_glsl(x.name)) & engine::resource::Format::Shader);

						const engine::resource::FormatMask matching_formats = available_formats & x.formats;
						if (!matching_formats.unique())
						{
							debug_fail("more than one formats matches '", x.name, "'");
						}
						else if (matching_formats & engine::resource::Format::Armature)
						{
							read_arm(x.name, "res/" + x.name + ".arm", x.callback);
						}
						else if (matching_formats & engine::resource::Format::Ini)
						{
							std::string filename = (check_if_ini(x.name) == 2 ? "res/" : "") + x.name + ".ini";
							read_ini(x.name, filename, x.callback);
						}
						else if (matching_formats & engine::resource::Format::Json)
						{
							std::string filename = (check_if_json(x.name) == 2 ? "res/" : "") + x.name + ".json";
							read_json(x.name, filename, x.callback);
						}
						else if (matching_formats & engine::resource::Format::Level)
						{
							read_lvl(x.name, "res/" + x.name + ".lvl", x.callback);
						}
						else if (matching_formats & engine::resource::Format::Placeholder)
						{
							read_msh(x.name, "res/" + x.name + ".msh", x.callback);
						}
						else if (matching_formats & engine::resource::Format::Png)
						{
							read_png(x.name, "res/" + x.name + ".png", x.callback);
						}
						else if (matching_formats & engine::resource::Format::Shader)
						{
							read_glsl(x.name, "res/gfx/" + x.name + ".glsl", x.callback);
						}
						else if (matching_formats)
						{
							debug_fail("unknown file format for '", x.name, "'");
						}
						else if (x.formats & engine::resource::Format::None)
						{
							no_read(x.name, x.name, x.callback);
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

	core::async::Thread renderThread;
	volatile bool active = false;
	core::sync::Event<true> event;

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
	namespace resource
	{
		namespace reader
		{
			void create()
			{
				debug_assert(!active);

				active = true;
				renderThread = core::async::Thread{ run };
			}

			void destroy()
			{
				debug_assert(active);

				active = false;
				event.set();

				renderThread.join();
			}

			void post_read(std::string name, void (* callback)(std::string name, Structurer && structurer), FormatMask formats)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRead>, std::move(name), callback, formats);
				debug_assert(res);
				if (res)
				{
					event.set();
				}
			}
		}
	}
}
