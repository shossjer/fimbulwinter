
#include "player.hpp"

#include <engine/graphics/events.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/hid/input.hpp>

namespace
{
	using engine::hid::Context;
	using engine::hid::Device;
	using engine::hid::Input;

	constexpr std::size_t this_object = 17;

	class Controller : public Context
	{
		float y = 0.f;

		bool translate(const Input & input)
		{
			switch (input.getState())
			{
			case Input::State::DOWN:
			{
				switch (input.getButton())
				{
				case Input::Button::KEY_ARROWDOWN:
					y -= .1f;
					break;
				case Input::Button::KEY_ARROWUP:
					y += .1f;
					break;
				case Input::Button::KEY_SPACEBAR:
					y = 0.f;
					break;
				default:
					return false;
				}
				const engine::graphics::ModelMatrixMessage message = {
					this_object,
					core::maths::Matrix4x4f(1.f, 0.f, 0.f, 0.f,
					                        0.f, 1.f, 0.f, y,
					                        0.f, 0.f, 1.f, 0.f,
					                        0.f, 0.f, 0.f, 1.f)
				};
				post_message(message);
				return true;
			}
			default:
				return false;
			}
		}
	} controller;

	Device device = {&controller};
}

namespace gameplay
{
	namespace player
	{
		void create()
		{
			const engine::graphics::Point point = {
				core::maths::Matrix4x4f(1.f, 0.f, 0.f, 0.f,
				                        0.f, 1.f, 0.f, 0.f,
				                        0.f, 0.f, 1.f, 0.f,
				                        0.f, 0.f, 0.f, 1.f),
				8.f
			};
			engine::graphics::renderer::add(this_object, point);
			engine::hid::add(this_object, device);
		}
		void destroy()
		{
			engine::hid::remove(this_object);
		}
	}
}
