
#include <core/debug.hpp>
#include <engine/application/window.hpp>

#include <gameplay/game-menu.hpp>
#include <gameplay/input.hpp>
// #include <gameplay/level.hpp>

#include <config.h>

#if WINDOW_USE_USER32
#include <windows.h>
#endif

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

	gameplay::looper::create();
	gameplay::input::create();
	gameplay::gamemenu::create();
	// gameplay::level::create("res/box.lvl");

	const int ret = engine::application::window::execute();

	debug_printline(0xffffffff, "loop is no more");

	// gameplay::level::destroy();
	gameplay::gamemenu::destroy();
	gameplay::input::destroy();
	gameplay::looper::destroy();

	engine::application::window::destroy(hInstance);

	return ret;
}
#elif WINDOW_USE_X11
int main(const int argc, const char *const argv[])
{
	engine::application::window::create();

	gameplay::looper::create();
	gameplay::input::create();
	gameplay::gamemenu::create();
	// gameplay::level::create("res/box.lvl");

	const int ret = engine::application::window::execute();

	debug_printline(0xffffffff, "loop is no more");

	// gameplay::level::destroy();
	gameplay::gamemenu::destroy();
	gameplay::input::destroy();
	gameplay::looper::destroy();

	engine::application::window::destroy();

	return ret;
}
#endif
