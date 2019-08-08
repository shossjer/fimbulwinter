
#ifndef ENGINE_HID_UI_HPP
#define ENGINE_HID_UI_HPP

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/hid/input.hpp"

#include <vector>

namespace engine
{
namespace hid
{
namespace ui
{
	void create();

	void destroy();

	void post_add_context(
		engine::Asset context,
		std::vector<engine::Asset> states);
	void post_remove_context(
		engine::Asset context);
}
}
}

#endif /* ENGINE_HID_UI_HPP */
