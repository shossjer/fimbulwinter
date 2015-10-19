
#include <core/debug.hpp>
#include <engine/application/window.hpp>
#include <engine/graphics/renderer.hpp>

int main(const int argc, const char *const argv[])
{
	engine::application::window::create();
	engine::graphics::renderer::create();

	const int ret = engine::application::window::execute();

	debug_printline(0xffffffff, "loop is no more");

	engine::graphics::renderer::destroy();
	engine::application::window::destroy();
	
	return ret;
}
