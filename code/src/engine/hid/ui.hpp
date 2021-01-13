#pragma once

#include "engine/Command.hpp"
#include "engine/hid/input.hpp"
#include "engine/Token.hpp"

#include <vector>

namespace engine
{
namespace hid
{
	class ui
	{
	public:
		~ui();
		ui();
	};

	void post_add_context(
		ui & ui,
		engine::Token context,
		std::vector<engine::Token> states);
	void post_remove_context(
		ui & ui,
		engine::Token context);

	void post_set_state(
		ui & ui,
		engine::Token context,
		engine::Token state);

	void post_add_device(
		ui & ui,
		engine::Token context,
		int id);
	void post_remove_device(
		ui & ui,
		engine::Token context,
		int id);

	void post_add_axis_move(
		ui & ui,
		engine::Token mapping,
		engine::hid::Input::Axis code,
		engine::Command command_x,
		engine::Command command_y);
	void post_add_axis_tilt(
		ui & ui,
		engine::Token mapping,
		engine::hid::Input::Axis code,
		engine::Command command_min,
		engine::Command command_max);
	void post_add_button_press(
		ui & ui,
		engine::Token mapping,
		engine::hid::Input::Button code,
		engine::Command command);
	void post_add_button_release(
		ui & ui,
		engine::Token mapping,
		engine::hid::Input::Button code,
		engine::Command command);

	void post_bind(
		ui & ui,
		engine::Token context,
		engine::Token state,
		engine::Token mapping,
		void (* callback)(engine::Command command, float value, void * data),
		void * data);
	void post_unbind(
		ui & ui,
		engine::Token context,
		engine::Token state,
		engine::Token mapping);
}
}
