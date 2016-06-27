
#include "game-menu.hpp"

#include <core/debug.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/hid/input.hpp>

namespace
{
	using engine::hid::Context;
	using engine::hid::Device;
	using engine::hid::Input;

	constexpr std::size_t this_object = 11;
	// constexpr std::size_t continue_object = 12; // not used
	// constexpr std::size_t exit_object = 13; // not used

	Device closed_dev;
	Device opened_dev;

	class Closed : public Context
	{
		bool translate(const Input & input) override
		{
			if (input.getState() == Input::State::DOWN)
			{
				if (input.getButton() == Input::Button::KEY_ESCAPE)
				{
					engine::hid::replace(this_object, opened_dev);
					engine::hid::set_focus(this_object);
					// const engine::graphics::RectangleMessage window_rectangle = {
					// 	this_object, {
					// 		0.1f, 0.1f, 0.2f,
					// 		200.f, 100.f,
					// 		-100.f, -50.f, 0.f
					// 	}
					// };
					// engine::graphics::post_message(window_rectangle);
					// const engine::graphics::RectangleMessage continue_rectangle = {
					// 	continue_object, {
					// 		0.1f, 0.2f, 0.1f,
					// 		100.f, 20.f,
					// 		-50.f, -25.f, 0.1f
					// 	}
					// };
					// engine::graphics::post_message(continue_rectangle);
					// const engine::graphics::RectangleMessage exit_rectangle = {
					// 	exit_object, {
					// 		0.2f, 0.1f, 0.1f,
					// 		100.f, 20.f,
					// 		-50.f, 5.f, 0.1f
					// 	}
					// };
					// engine::graphics::post_message(exit_rectangle);
					return true;
				}
			}
			return false;
		}
	} closed;

	class Opened : public Context
	{
		bool translate(const Input & input) override
		{
			if (input.getState() == Input::State::DOWN)
			{
				if (input.getButton() == Input::Button::KEY_ESCAPE)
				{
					// const engine::graphics::RemoveMessage exit_remove = {
					// 	exit_object
					// };
					// engine::graphics::post_message(exit_remove);
					// const engine::graphics::RemoveMessage continue_remove = {
					// 	continue_object
					// };
					// engine::graphics::post_message(continue_remove);
					// const engine::graphics::RemoveMessage window_remove = {
					// 	this_object
					// };
					// engine::graphics::post_message(window_remove);
					engine::hid::replace(this_object, closed_dev);
					return true;
				}
			}
			return true;
		}
	} opened;
}

namespace gameplay
{
	namespace gamemenu
	{
		void create()
		{
			closed_dev.context = &closed;
			opened_dev.context = &opened;

			engine::hid::add(this_object, closed_dev);
		}
		void destroy()
		{
			engine::hid::remove(this_object);
		}
	}
}
