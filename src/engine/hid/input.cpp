
#include "input.hpp"

#include <core/debug.hpp>
#include <engine/graphics/events.hpp>

namespace engine
{
	namespace hid
	{
		void dispatch(const Input & input)
		{
			// for (std::size_t i = 0; i < ncontexts; i++)
			// {
			// 	if (contexts[i]->translate(input)) break;
			// }

			static float y = 0.f;

			switch (input.getState())
			{
			case Input::State::DOWN:
				switch (input.getButton())
				{
				case Input::Button::KEY_ARROWDOWN:
				{
					y -= .1f;
					const engine::graphics::ModelViewMessage message = {
						17,
						core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
						                     0.f, 1.f, 0.f, y,
						                     0.f, 0.f, 1.f, 0.f,
						                     0.f, 0.f, 0.f, 1.f)
					};
					engine::graphics::post_message(message);
					break;
				}
				case Input::Button::KEY_ARROWUP:
				{
					y += .1f;
					const engine::graphics::ModelViewMessage message = {
						17,
						core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
						                     0.f, 1.f, 0.f, y,
						                     0.f, 0.f, 1.f, 0.f,
						                     0.f, 0.f, 0.f, 1.f)
					};
					engine::graphics::post_message(message);
					break;
				}
				case Input::Button::KEY_SPACEBAR:
				{
					y = 0.f;
					const engine::graphics::ModelViewMessage message = {
						17,
						core::maths::Matrixf(1.f, 0.f, 0.f, 0.f,
						                     0.f, 1.f, 0.f, y,
						                     0.f, 0.f, 1.f, 0.f,
						                     0.f, 0.f, 0.f, 1.f)
					};
					engine::graphics::post_message(message);
					break;
				}
				default:
					;
				}
				break;
			default:
				;
			}
		}
	}
}
