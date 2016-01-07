
#include "player.hpp"

#include <engine/graphics/events.hpp>
#include <engine/hid/input.hpp>

namespace
{
	using engine::hid::Context;
	using engine::hid::Device;
	using engine::hid::Input;

	constexpr std::size_t player_object = 17;

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
				const engine::graphics::ModelViewMessage message = {
					player_object,
					core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
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
			engine::hid::add(player_object, device);
		}
		void destroy()
		{
			engine::hid::remove(player_object);
		}
	}
}
