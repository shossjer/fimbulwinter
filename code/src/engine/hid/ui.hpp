
#ifndef ENGINE_HID_UI_HPP
#define ENGINE_HID_UI_HPP

#include "engine/Asset.hpp"
#include "engine/Command.hpp"
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

	void post_add_device(
		engine::Asset context,
		int id);
	void post_remove_device(
		engine::Asset context,
		int id);

	void post_add_axis_tilt(
		engine::Entity mapping,
		engine::hid::Input::Axis code,
		engine::Command command_min,
		engine::Command command_max);
	void post_add_button_press(
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command);
	void post_add_button_release(
		engine::Entity mapping,
		engine::hid::Input::Button code,
		engine::Command command);

	void post_bind(
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping,
		engine::Entity callback);
	void post_unbind(
		engine::Asset context,
		engine::Asset state,
		engine::Entity mapping);
}
}
}

#endif /* ENGINE_HID_UI_HPP */
