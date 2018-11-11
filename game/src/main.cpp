
#include "core/debug.hpp"

#include "engine/application/window.hpp"
#include "engine/hid/ui.hpp"
#include "engine/resource/reader.hpp"
#include "engine/resource/writer.hpp"

#include "gameplay/gamestate.hpp"

#include "settings.hpp"

#include "config.h"

#if WINDOW_USE_ANDROID
# include "android_native_app_glue.h"
#elif WINDOW_USE_USER32
# include <windows.h>
#endif

namespace engine
{
	namespace console
	{
		extern void create();
		extern void destroy();
	}

	namespace graphics
	{
		namespace renderer
		{
			extern void create();
			extern void destroy();
		}

		namespace viewer
		{
			extern void create();
			extern void destroy();
		}
	}

	namespace gui
	{
		extern void create();
		extern void destroy();
	}

	namespace physics
	{
		extern void create();
		extern void destroy();
	}

	namespace replay
	{
		extern void create();
		extern void destroy();
	}

	namespace resource
	{
		namespace reader
		{
			extern void create();
			extern void destroy();
		}

		namespace writer
		{
			extern void create();
			extern void destroy();
		}
	}
}

namespace gameplay
{
	namespace looper
	{
		extern void create();
		extern void destroy();
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

#if WINDOW_USE_ANDROID
void android_main(struct android_app * state)
{
	// state->userData = &engine;
	// state->onAppCmd = engine_handle_cmd;
	// state->onInputEvent = engine_handle_input;

	
}
#elif WINDOW_USE_USER32
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
	::engine::resource::reader::create();
	::engine::resource::writer::create();

	::engine::resource::reader::post_read("settings", read_settings_callback, ::engine::resource::Format::Ini | ::engine::resource::Format::Json | ::engine::resource::Format::None);
	while (settings_lock < 1);

	::engine::resource::writer::post_write(std::string("settings") + get_settings_extension(), write_settings_callback);
	while (settings_lock < 2);

	engine::application::window::create(hInstance, nCmdShow, settings.application);

	::engine::console::create();
	;;engine::replay::create();
	::engine::physics::create();
	::engine::graphics::renderer::create(settings.graphics.renderer_type);
	::engine::graphics::viewer::create();
	::engine::gui::create();
	::engine::hid::ui::create();
	::gameplay::gamestate::create();
	::gameplay::looper::create();

	const int ret = engine::application::window::execute();

	debug_printline("loop is no more");

	::gameplay::looper::destroy();
	::gameplay::gamestate::destroy();
	::engine::hid::ui::destroy();
	::engine::gui::destroy();
	::engine::graphics::viewer::destroy();
	::engine::graphics::renderer::destroy();
	::engine::physics::destroy();
	::engine::replay::destroy();
	::engine::console::destroy();

	engine::application::window::destroy(hInstance);

	::engine::resource::writer::destroy();
	::engine::resource::reader::destroy();

	return ret;
}
#elif WINDOW_USE_X11
int main(const int argc, const char *const argv[])
{
	std::string thdo = "hello";

	::engine::resource::reader::create();
	::engine::resource::writer::create();

	::engine::resource::reader::post_read("settings", read_settings_callback, ::engine::resource::Format::Ini | ::engine::resource::Format::Json | ::engine::resource::Format::None);
	while (settings_lock < 1);

	::engine::resource::writer::post_write(std::string("settings") + get_settings_extension(), write_settings_callback);
	while (settings_lock < 2);

	engine::application::window::create(settings.application);

	::engine::console::create();
	;;engine::replay::create();
	::engine::physics::create();
	::engine::graphics::renderer::create(settings.graphics.renderer_type);
	::engine::graphics::viewer::create();
	::engine::gui::create();
	::engine::hid::ui::create();
	::gameplay::gamestate::create();
	::gameplay::looper::create();

	const int ret = engine::application::window::execute();

	debug_printline("loop is no more");

	::gameplay::looper::destroy();
	::gameplay::gamestate::destroy();
	::engine::hid::ui::destroy();
	::engine::gui::destroy();
	::engine::graphics::viewer::destroy();
	::engine::graphics::renderer::destroy();
	::engine::physics::destroy();
	::engine::replay::destroy();
	::engine::console::destroy();

	engine::application::window::destroy();

	::engine::resource::writer::destroy();
	::engine::resource::reader::destroy();

	return ret;
}
#endif
