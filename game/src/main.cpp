
#include "core/debug.hpp"

#include "engine/animation/mixer.hpp"
#include "engine/application/window.hpp"
#include "engine/audio/system.hpp"
#include "engine/console.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/hid/devices.hpp"
#include "engine/hid/ui.hpp"
#include "engine/physics/physics.hpp"
#include "engine/replay/writer.hpp"
#include "engine/resource/reader.hpp"
#include "engine/resource/writer.hpp"

#include "gameplay/gamestate.hpp"
#include "gameplay/looper.hpp"

#include "settings.hpp"

#include "config.h"

#if WINDOW_USE_USER32
# include <windows.h>
#endif

namespace engine
{
	namespace application
	{
		extern int execute(window & window);
	}
}

namespace
{
	std::atomic_int settings_lock(0);
	settings_t settings;

	struct ReadSettings
	{
		void operator () (core::IniStructurer && x)
		{
			x.read(settings);
		}

		void operator () (core::JsonStructurer && x)
		{
			x.read(settings);
		}

		void operator () (core::NoSerializer && x)
		{
			debug_printline("NO SERIALIZER FOR ", x.filename);
		}

		template <typename T>
		void operator () (T && x)
		{
			debug_unreachable();
		}
	};

	struct WriteSettings
	{
		void operator () (core::IniSerializer & x)
		{
			x.write(settings);
		}

		void operator () (core::JsonSerializer & x)
		{
			x.write(settings);
		}

		template <typename T>
		void operator () (T & x)
		{
			debug_unreachable();
		}
	};

	void read_settings_callback(std::string name, engine::resource::reader::Structurer && structurer)
	{
		utility::visit(ReadSettings{}, std::move(structurer));

		settings_lock++;
	}

	void write_settings_callback(std::string name, engine::resource::writer::Serializer & serializer)
	{
		utility::visit(WriteSettings{}, serializer);

		settings_lock++;
	}

	std::string get_settings_extension()
	{
		switch (settings.general.settings_format)
		{
		case engine::resource::Format::Ini:
			return ".ini";
		case engine::resource::Format::Json:
			return ".json";
		default:
			debug_fail();
			return ".wtf";
		}
	}
}

#if WINDOW_USE_USER32
# if MODE_DEBUG
int main(const int argc, const char *const argv[])
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	int nCmdShow = SW_SHOW;

	debug_assert(hInstance);
	if (hInstance == nullptr)
		return -1;
# else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
# endif
	engine::resource::reader reader;
	engine::resource::writer writer;

	reader.post_read("settings", read_settings_callback, ::engine::resource::Format::Ini | ::engine::resource::Format::Json | ::engine::resource::Format::None);
	while (settings_lock < 1);

	writer.post_write(std::string("settings") + get_settings_extension(), write_settings_callback);
	while (settings_lock < 2);

	engine::console console(engine::application::close);

	engine::record record;
	engine::audio::System audio;

	engine::application::window window(hInstance, nCmdShow, settings.application);

	engine::hid::ui ui;
	engine::hid::devices devices(window, ui, settings.hid.hardware_input);

	engine::graphics::renderer renderer(window, reader, settings.graphics.renderer_type);
	engine::graphics::viewer viewer(renderer);
	engine::physics::simulation simulation(renderer, viewer);
	engine::animation::mixer mixer(renderer, simulation);

	gameplay::gamestate gamestate(mixer, audio, renderer, viewer, ui, simulation, record, reader);
	gameplay::looper looper(mixer, renderer, viewer, ui, simulation, gamestate);

	renderer.set_dependencies(&gamestate);
	window.set_dependencies(viewer, devices, ui);

	return execute(window);
}
#elif WINDOW_USE_X11
int main(const int argc, const char *const argv[])
{
	engine::resource::reader reader;
	engine::resource::writer writer;

	reader.post_read("settings", read_settings_callback, ::engine::resource::Format::Ini | ::engine::resource::Format::Json | ::engine::resource::Format::None);
	while (settings_lock < 1);

	writer.post_write(std::string("settings") + get_settings_extension(), write_settings_callback);
	while (settings_lock < 2);

	engine::console console(engine::application::close);

	engine::record record;
	engine::audio::System audio;

	engine::application::window window(settings.application);

	engine::hid::ui ui;
	engine::hid::devices devices(window, ui, settings.hid.hardware_input);

	engine::graphics::renderer renderer(window, reader, settings.graphics.renderer_type);
	engine::graphics::viewer viewer(renderer);
	engine::physics::simulation simulation(renderer, viewer);
	engine::animation::mixer mixer(renderer, simulation);

	gameplay::gamestate gamestate(mixer, audio, renderer, viewer, ui, simulation, record, reader);
	gameplay::looper looper(mixer, renderer, viewer, ui, simulation, gamestate);

	renderer.set_dependencies(&gamestate);
	window.set_dependencies(viewer, devices, ui);

	return execute(window);
}
#endif
