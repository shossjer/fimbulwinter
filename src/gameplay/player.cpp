
#include "player.hpp"

#include <gameplay/CharacterState.hpp>

#include <engine/graphics/events.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/hid/input.hpp>

#include <core/maths/Vector.hpp>

#include <utility/type_traits.hpp>

#include <array>

namespace
{
	using engine::hid::Context;
	using engine::hid::Device;
	using engine::hid::Input;

	constexpr std::size_t this_object = 17;

	template<typename>
	struct MapWrapper;

	template<std::size_t...Ns>
	struct MapWrapper<std::index_sequence<Ns...>>
	{
		std::array<bool, sizeof...(Ns)> keyMap;

		MapWrapper() : keyMap{(Ns, false)...}
		{}
	};

	gameplay::CharacterState emptyState;

	class StateHID : public Context
	{
		using KeyMapT = MapWrapper<std::make_index_sequence<256>>;

	public:

		static const signed int DIR_LEFT = 1;
		static const signed int DIR_RIGHT = -1;
		static const signed int DIR_IN = 4;
		static const signed int DIR_OUT = -4;

		signed int dirFlags;

		KeyMapT keyMap;

		gameplay::CharacterState * characterState;

	public:

		StateHID(gameplay::CharacterState * player) : dirFlags(0), characterState(player)
		{
		}

	public:

		void inputJump()
		{
			if (!this->characterState->grounded)
				return;

			this->characterState->fallVel = 10.f;
		}

		void update()
		{
			//if (!this->characterState.grounded)
			//{
			//	this->characterState.movementState = CharacterState::MovementState::NONE;
			//	return;
			//}

			switch (this->dirFlags)	// check key press's for movement
			{
			case 0:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::NONE;
				break;

			case DIR_LEFT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::LEFT;
				break;

			case DIR_IN + DIR_LEFT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::LEFT_UP;
				break;

			case DIR_IN:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::UP;
				break;

			case DIR_IN + DIR_RIGHT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::RIGHT_UP;
				break;

			case DIR_RIGHT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::RIGHT;
				break;

			case DIR_OUT + DIR_RIGHT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::RIGHT_DOWN;
				break;

			case DIR_OUT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::DOWN;
				break;

			case DIR_OUT + DIR_LEFT:

				this->characterState->movementState = ::gameplay::CharacterState::MovementState::LEFT_DOWN;
				break;
			}
		}

		bool translate(const Input & input)
		{
			switch (input.getState())
			{
			case Input::State::DOWN:

				if (this->keyMap.keyMap[(unsigned int)input.getButton()])
					return false;

				this->keyMap.keyMap[(unsigned int)input.getButton()] = true;

				switch (input.getButton())
				{
				case Input::Button::KEY_ARROWDOWN:
				{
					this->dirFlags += StateHID::DIR_OUT;
					break;
				}
				case Input::Button::KEY_ARROWUP:
				{
					this->dirFlags += StateHID::DIR_IN;
					break;
				}

				case Input::Button::KEY_ARROWLEFT:
				{
					this->dirFlags += StateHID::DIR_LEFT;
					break;
				}
				case Input::Button::KEY_ARROWRIGHT:
				{
					this->dirFlags += StateHID::DIR_RIGHT;
					break;
				}

				case Input::Button::KEY_SPACEBAR:
				{
					this->inputJump();
					break;
				}
				default:
					;
				}
				break;

			case Input::State::UP:

				if (!this->keyMap.keyMap[(unsigned int)input.getButton()])
					return false;

				this->keyMap.keyMap[(unsigned int)input.getButton()] = false;

			//	if (!this->characterState->grounded)
			//		return true;

				switch (input.getButton())
				{
				case Input::Button::KEY_ARROWDOWN:
				{
					this->dirFlags -= StateHID::DIR_OUT;
					break;
				}
				case Input::Button::KEY_ARROWUP:
				{
					this->dirFlags -= StateHID::DIR_IN;
					break;
				}

				case Input::Button::KEY_ARROWLEFT:
				{
					this->dirFlags -= StateHID::DIR_LEFT;
					break;
				}
				case Input::Button::KEY_ARROWRIGHT:
				{
					this->dirFlags -= StateHID::DIR_RIGHT;
					break;
				}
				default:
					break;
				}
			default:
				;
			}

			this->update();
			this->characterState->update();

			return true;
		}

	}	stateHID(&emptyState);


	Device testDevice;
}

//namespace engine
//{
//	namespace player
//	{
//		void update()
//		{
//			stateHID.update();
//
//			engine::physics::MoveResult res = engine::physics::update(0, playerState.movement(), playerState.fallVel);
//
//			playerState.grounded = res.isGrounded;
//			playerState.fallVel = res.fallVel;
//		}
//
//		//void input(const std::size_t object, const bool grounded)
//		//{
//		//	playerState.grounded = grounded;
//		//}
//	}
//}

namespace gameplay
{
	namespace input
	{
		void setup(CharacterState * player)
		{
			stateHID.characterState = player;
		}

		void setup()
		{
			stateHID.characterState = &emptyState;
		}

		//void update()
		//{
		//	stateHID.update();
		//}

		void create()
		{
			testDevice.context = &stateHID;

			engine::hid::add(this_object, testDevice);
		}

		void destroy()
		{
			engine::hid::remove(this_object);
		}
	}
}