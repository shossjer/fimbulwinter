
#include <gameplay/characters.hpp>
#include <gameplay/CharacterState.hpp>
#include <gameplay/effects.hpp>
#include <gameplay/input.hpp>
#include <gameplay/context/Context.hpp>

#include <engine/Entity.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/hid/input.hpp>
#include <engine/physics/queries.hpp>

#include <unordered_map>


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

		//engine::Entity effectId;
		std::unordered_map<Input::Button, engine::Entity> activeEffects;

		void remEffect(const Input::Button key)
		{
			const auto itr = activeEffects.find(key);

			if (itr != activeEffects.end())
			{
				effects::remove(itr->second);
				activeEffects.erase(itr);
			}

			//if (this->effectId!= engine::Entity::INVALID)
			//{
			//	effects::remove(this->effectId);
			//	this->effectId = engine::Entity::INVALID;
			//}
		}

		engine::Entity camera;

	public:
		Player() :
			jumpPressed(false),
			camera(engine::Entity::create())
		{
			engine::graphics::viewer::add(camera,
			                              engine::graphics::viewer::camera(core::maths::Quaternionf(1.f, 0.f, 0.f, 0.f),
			                                                               core::maths::Vector3f(0.f, 0.f, 0.f)));
		}
		~Player()
		{
			engine::graphics::viewer::remove(camera);
		}

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

			Point pos;
			Vector vec;

			engine::physics::query::positionOf(id, pos, vec);

			vec[0] *= 0.25f;
			vec[1] *= 0.25f;
			vec[2] *= 0.25f;

			const core::maths::Vector3f goal{pos[0] + vec[0], pos[1] + vec[1], 20.f};
			static core::maths::Vector3f current{0.f, 0.f, 20.f};
			const auto delta = goal - current;

			current += delta * .1f;
			engine::graphics::viewer::update(camera, engine::graphics::viewer::translation(current));
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
					this->activeEffects.emplace(Input::Button::MOUSE_LEFT, gameplay::effects::create(gameplay::effects::Type::PLAYER_MOUSE_FORCE, player::get()));
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
					this->activeEffects.emplace(Input::Button::KEY_SPACEBAR, gameplay::effects::create(gameplay::effects::Type::PLAYER_GRAVITY, player::get()));
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
					remEffect(Input::Button::MOUSE_LEFT);
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
					remEffect(Input::Button::KEY_SPACEBAR);
					break;
				}
			default:
				; // do nothing
			}
		}
	};
}
}
