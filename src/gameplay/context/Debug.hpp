
#include <gameplay/context/Context.hpp>

#include <engine/Entity.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/hid/input.hpp>

namespace gameplay
{

namespace context
{
	using engine::hid::Input;
	/**
	 *	Looks for movement (Keyboard + Mouse)
	 */
	class Debug : public Context
	{
	private:
		Vec2 vec;

		engine::Entity camera;

	public:
		Debug() :
			camera(engine::Entity::create())
		{
			engine::graphics::viewer::add(camera,
			                              engine::graphics::viewer::camera(core::maths::Quaternionf(1.f, 0.f, 0.f, 0.f),
			                                                               core::maths::Vector3f(0.f, 0.f, 0.f)));
		}
		~Debug()
		{
			engine::graphics::viewer::remove(camera);
		}

	public:
		/**
		 *	update camera position based on player input
		 */
		void updateInput() override
		{
			// check DebugState input
			switch (this->dirFlags)
			{
			case 0:

				vec = { { 0.f, 0.f } };
				//character.movementState = ::gameplay::CharacterState::MovementState::NONE;
				break;

			case DIR_LEFT:

				//character.movementState = ::gameplay::CharacterState::MovementState::LEFT;
				vec = { { -1.f, 0.f } };
				break;

			case DIR_IN + DIR_LEFT:

				//character.movementState = ::gameplay::CharacterState::MovementState::LEFT_UP;
				vec = { { -1.f, -1.f } };
				break;

			case DIR_IN:

				//character.movementState = ::gameplay::CharacterState::MovementState::UP;
				vec = { { 0.f, -1.f } };
				break;

			case DIR_IN + DIR_RIGHT:

				//character.movementState = ::gameplay::CharacterState::MovementState::RIGHT_UP;
				vec = { { 1.f, -1.f } };
				break;

			case DIR_RIGHT:

				//character.movementState = ::gameplay::CharacterState::MovementState::RIGHT;
				vec = { { 1.f, 0.f } };
				break;

			case DIR_OUT + DIR_RIGHT:

				//character.movementState = ::gameplay::CharacterState::MovementState::RIGHT_DOWN;
				vec = { { 1.f, 1.f } };
				break;

			case DIR_OUT:

				//character.movementState = ::gameplay::CharacterState::MovementState::DOWN;
				vec = { { 0.f, 1.f } };
				break;

			case DIR_OUT + DIR_LEFT:

				//character.movementState = ::gameplay::CharacterState::MovementState::LEFT_DOWN;
				vec = { { -1.f, 1.f } };
				break;
			}

			//

		}

		void updateCamera() override
		{
			engine::graphics::viewer::update(camera,
			                                 engine::graphics::viewer::translate(core::maths::Vector3f(vec[0], 0.f, vec[1])));
			engine::graphics::viewer::set_active_3d(camera); // this should not be done every time
		}

		void onMove(const Input & input) override
		{

		}

		void onDown(const Input & input) override
		{
			switch (input.getButton())
			{
				case Input::Button::MOUSE_LEFT:
				{
					break;
				}
				case Input::Button::KEY_ARROWDOWN:
				{
					this->dirFlags += Context::DIR_OUT;
					break;
				}
				case Input::Button::KEY_ARROWUP:
				{
					this->dirFlags += Context::DIR_IN;
					break;
				}

				case Input::Button::KEY_ARROWLEFT:
				{
					this->dirFlags += Context::DIR_LEFT;
					break;
				}
				case Input::Button::KEY_ARROWRIGHT:
				{
					this->dirFlags += Context::DIR_RIGHT;
					break;
				}

				case Input::Button::KEY_SPACEBAR:
				{
				//	this->effectId = gameplay::effects::create(gameplay::effects::Type::PLAYER_GRAVITY, player::get());
					break;
				}
			default:
				; // do nothing
			}
		}

		void onUp(const Input & input) override
		{
			switch (input.getButton())
			{
				case Input::Button::MOUSE_LEFT:
				{
					break;
				}
				case Input::Button::KEY_ARROWDOWN:
				{
					this->dirFlags -= Context::DIR_OUT;
					break;
				}
				case Input::Button::KEY_ARROWUP:
				{
					this->dirFlags -= Context::DIR_IN;
					break;
				}

				case Input::Button::KEY_ARROWLEFT:
				{
					this->dirFlags -= Context::DIR_LEFT;
					break;
				}
				case Input::Button::KEY_ARROWRIGHT:
				{
					this->dirFlags -= Context::DIR_RIGHT;
					break;
				}
				case Input::Button::KEY_SPACEBAR:
				{
				//	gameplay::effects::remove(this->effectId);
					break;
				}
			default:
				; // do nothing
			}
		}
	};
}
}
