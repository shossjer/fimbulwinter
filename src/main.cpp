
#include <engine/application/window.hpp>

int main(const int argc, const char *const argv[])
{
	engine::application::window::create();

	const int ret = engine::application::window::execute();

	engine::application::window::destroy();
	
	return ret;
}
