
#ifndef GAMEPLAY_UI_HPP
#define GAMEPLAY_UI_HPP

#include <engine/Asset.hpp>
#include <engine/Entity.hpp>
#include <engine/hid/input.hpp>

namespace gameplay
{
namespace ui
{
	void create();

	void destroy();

	void post_add_context(
		engine::Asset context);
	void post_remove_context(
		engine::Asset context);
	void post_activate_context(
		engine::Asset context);

	void post_add_contextswitch(
		engine::Entity entity,
		engine::hid::Input::Button button,
		engine::Asset context);
	void post_add_flycontrol(
		engine::Entity entity);
	void post_add_pancontrol(
		engine::Entity entity);
	void post_remove(
		engine::Entity entity);

	void post_bind(
		engine::Asset context,
		engine::Entity entity,
		int priority);
	void post_unbind(
		engine::Asset context,
		engine::Entity entity);
}
}

#endif /* GAMEPLAY_UI_HPP */
