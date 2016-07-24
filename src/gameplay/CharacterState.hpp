
#ifndef GAMEPLAY_CHARACTER_STATE_HPP
#define GAMEPLAY_CHARACTER_STATE_HPP

#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>
#include <engine/physics/defines.hpp>


namespace gameplay
{
	class CharacterState
	{
	public:

		//const engine::Entity id;

		using Type = engine::physics::Vec2;

		enum MovementState
		{
			NONE,	// no active movement (can still be falling etc)

			LEFT,
			LEFT_UP,
			UP,
			RIGHT_UP,
			RIGHT,
			RIGHT_DOWN,
			DOWN,
			LEFT_DOWN
		}   movementState;

	public:

		bool grounded;
		float fallVel;
		Type vec;

		CharacterState() : //unsigned int id) : id(id), 
			movementState(NONE), grounded(false), fallVel(0.f), vec{{0.f, 0.f}}
		{
		}

	public:


		Type movement() const
		{
			return this->vec;
		}

		void update()
		{
			switch (this->movementState)	// check key press's for movement
			{
			case LEFT:

				vec = {{ -1.f, 0.f }};
				break;

			case LEFT_UP:

				vec = {{ -1.f, -1.f }};
				break;

			case UP:

				vec = {{ 0.f, -1.f }};
				break;

			case RIGHT_UP:

				vec = {{ 1.f, -1.f }};
				break;

			case RIGHT:
				
				vec = {{ 1.f, 0.f }};
				break;

			case RIGHT_DOWN:

				vec = {{ 1.f, 1.f }};
				break;

			case DOWN:

				vec = {{ 0.f, 1.f }};
				break;

			case LEFT_DOWN:

				vec = {{ -1.f, 1.f }};
				break;

			default:

				vec = {{ 0.f, 0.f }};
				break;
			}
		}

	};
}

#endif // GAMEPLAY_CHARACTER_STATE_HPP