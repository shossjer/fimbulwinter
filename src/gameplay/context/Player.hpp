
#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>
#include <gameplay/input.hpp>
#include <gameplay/context/Context.hpp>

#include <engine/hid/input.hpp>
#include <engine/physics/queries.hpp>


namespace gameplay
{

namespace context
{
	using engine::hid::Input;
	/**
	 *	Looks for player movement (Keyboard)
	 *	Looks for effect usage
	 *  Looks for mouse interaction
	 */
	class Player : public Context
	{
	private:

		bool jumpPressed;
		unsigned int effectId;

	public:

		Player() : jumpPressed(false), effectId(0)
		{}

	public:

		/**
		 *	Updates CharacterState of Player character
		 */
		void updateInput() override
		{
			// get player Character
			const engine::Entity id = player::get();

			::gameplay::CharacterState & character = characters::get(id);

			// check if jump button has been pressed
			if (this->jumpPressed)
			{
				this->jumpPressed = false;

				// attempt a jump!
				if (character.grounded)
				{
					character.fallVel = 6.f;
				}
			}

			switch (this->dirFlags)	// check key press's for movement
			{
			case 0:

				character.movementState = ::gameplay::CharacterState::MovementState::NONE;
				break;

			case DIR_LEFT:

				character.movementState = ::gameplay::CharacterState::MovementState::LEFT;
				break;

			case DIR_IN + DIR_LEFT:

				character.movementState = ::gameplay::CharacterState::MovementState::LEFT_UP;
				break;

			case DIR_IN:

				character.movementState = ::gameplay::CharacterState::MovementState::UP;
				break;

			case DIR_IN + DIR_RIGHT:

				character.movementState = ::gameplay::CharacterState::MovementState::RIGHT_UP;
				break;

			case DIR_RIGHT:

				character.movementState = ::gameplay::CharacterState::MovementState::RIGHT;
				break;

			case DIR_OUT + DIR_RIGHT:

				character.movementState = ::gameplay::CharacterState::MovementState::RIGHT_DOWN;
				break;

			case DIR_OUT:

				character.movementState = ::gameplay::CharacterState::MovementState::DOWN;
				break;

			case DIR_OUT + DIR_LEFT:

				character.movementState = ::gameplay::CharacterState::MovementState::LEFT_DOWN;
				break;
			}

			character.update();
		}

		/**
		 *	update camera position based on player character and movement
		 */
		void updateCamera() override
		{
			// 
			const engine::Entity id = player::get();

			if (id == engine::Entity::INVALID) return;

			engine::physics::query::Actor actor = engine::physics::query::load(id);

			const b2Vec2 pos = actor.body->GetPosition();
			b2Vec2 vec = actor.body->GetLinearVelocity();
			vec *= 0.25f;

			const b2Vec2 goal = pos + vec;

			const b2Vec2 current{ this->camera.getX(), this->camera.getY() };

			const b2Vec2 delta = goal - current;

			this->camera.position(current.x + delta.x*.1f, current.y + delta.y*.1f, 20.f);
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
					this->effectId = gameplay::effects::create(gameplay::effects::Type::PLAYER_GRAVITY, player::get());
					break;
				}
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
					gameplay::effects::remove(this->effectId);
					break;
				}
			}
		}

	};
}
}
