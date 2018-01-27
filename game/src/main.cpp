
#include <core/debug.hpp>

#include <engine/application/window.hpp>
#include "engine/hid/ui.hpp"

#include <gameplay/gamestate.hpp>

#include <config.h>

#if WINDOW_USE_USER32
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

	namespace resource
	{
		namespace loader
		{
			extern void create();
			extern void destroy();
		}

		namespace reader
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
	engine::application::window::create(hInstance, nCmdShow);

	::engine::console::create();
	::engine::resource::reader::create();
	::engine::resource::loader::create();
	::engine::physics::create();
	::engine::graphics::renderer::create();
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
	::engine::resource::loader::destroy();
	::engine::resource::reader::destroy();
	::engine::console::destroy();

	engine::application::window::destroy(hInstance);

	return ret;
}
#elif WINDOW_USE_X11
int main(const int argc, const char *const argv[])
{
	engine::application::window::create();

	::engine::console::create();
	::engine::resource::reader::create();
	::engine::resource::loader::create();
	::engine::physics::create();
	::engine::graphics::renderer::create();
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
	::engine::resource::loader::destroy();
	::engine::resource::reader::destroy();
	::engine::console::destroy();

	engine::application::window::destroy();

	return ret;
}
#endif
