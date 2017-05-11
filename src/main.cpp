
#include <core/debug.hpp>
#include <engine/application/window.hpp>
#include <gameplay/gamestate.hpp>
#include <gameplay/level.hpp>
#include <gameplay/ui.hpp>

#include <config.h>

#if WINDOW_USE_USER32
#include <windows.h>
#endif

namespace engine
{
namespace graphics
{
namespace renderer
{
	extern void create();
	extern void destroy();
}
}

namespace physics
{
	extern void create();
	extern void destroy();
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

	if (hInstance == nullptr)
		throw std::runtime_error("GetModuleHandle failed");
# else
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
# endif
	engine::application::window::create(hInstance, nCmdShow);

	::engine::physics::create();
	::engine::graphics::renderer::create();
	::gameplay::ui::create();
	::gameplay::gamestate::create();
	::gameplay::level::create("res/level.lvl");
	::gameplay::looper::create();

	const int ret = engine::application::window::execute();

	debug_printline(0xffffffff, "loop is no more");

	::gameplay::looper::destroy();
	::gameplay::level::destroy();
	::gameplay::gamestate::destroy();
	::gameplay::ui::destroy();
	::engine::graphics::renderer::destroy();
	::engine::physics::destroy();

	engine::application::window::destroy(hInstance);

	return ret;
}
#elif WINDOW_USE_X11
int main(const int argc, const char *const argv[])
{
	engine::application::window::create();

	::engine::physics::create();
	::engine::graphics::renderer::create();
	::gameplay::ui::create();
	::gameplay::gamestate::create();
	::gameplay::level::create("res/level.lvl");
	::gameplay::looper::create();

	const int ret = engine::application::window::execute();

	debug_printline(0xffffffff, "loop is no more");

	::gameplay::looper::destroy();
	::gameplay::level::destroy();
	::gameplay::gamestate::destroy();
	::gameplay::ui::destroy();
	::engine::graphics::renderer::destroy();
	::engine::physics::destroy();

	engine::application::window::destroy();

	return ret;
}
#endif
