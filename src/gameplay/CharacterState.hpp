
#ifndef GAMEPLAY_CHARACTER_STATE_HPP
#define GAMEPLAY_CHARACTER_STATE_HPP

#include "characters.hpp"

#include <engine/Entity.hpp>
#include <engine/physics/defines.hpp>
#include <engine/physics/queries.hpp>

// #include <core/debug.hpp>
#include <core/maths/Vector.hpp>

// #include <gameplay/input.hpp>

namespace gameplay
{
namespace characters
{
	/**
	 *	Should be used by gameplay::characters scope only
	 */
	class CharacterState
	{
	public:
		
		using Vector3f = core::maths::Vector3f;

	public:

	private:

		using Flag = unsigned int;

		static constexpr Flag BIT_GROUNDED = 0x1;

		Flag flags;

		Vector3f groundNormal;

	public:

		bool isGrounded() const { return (this->flags & BIT_GROUNDED) != 0; }

		const Vector3f & getGroundNormal() const { return this->groundNormal; }

	public:

		void setGrounded(const Vector3f & groundNormal)
		{
			this->groundNormal = groundNormal;
			this->flags |= BIT_GROUNDED;
		}

		void clrGrounded()
		{
			this->flags &=~BIT_GROUNDED;
		}

	public:

		float fallVel;
		Vector3f vec;
		// Type vec;
		// float angvel;

		CharacterState() : flags(0), fallVel(0.f), vec{0.f, 0.f, 0.f}
		// CharacterState() : //unsigned int id) : id(id), 
		// 	movementState(NONE), grounded(false), fallVel(0.f), vec{{0.f, 0.f}}, angvel(0.f)
		{
		}

	public:

		Vector3f movement() const
		{
			return this->vec;
		}

		void update(const MovementState state)
		{
			switch (state)	// check key press's for movement
			// Point pos;
			// Vector vec;
			// float angle;
			// engine::physics::query::positionOf(player::get(), pos, vec, angle);

			// const auto vx = std::cos(angle);
			// const auto vy = std::sin(angle);

			// switch (this->movementState)	// check key press's for movement
			{
			case LEFT:
				vec = { -1.f, 0.f, 0.f };
				// this->vec = {{ -vx, -vy }};
				break;
			case LEFT_UP:
				vec = { -1.f, -1.f, 0.f };
				break;
			case UP:
				vec = { 0.f, -1.f, 0.f };
				break;
			case RIGHT_UP:
				vec = { 1.f, -1.f, 0.f };
				break;
			case RIGHT:
				vec = { 1.f, 0.f, 0.f };
				// this->vec = {{ vx, vy }};
				break;
			case RIGHT_DOWN:
				vec = { 1.f, 1.f, 0.f };
				break;
			case DOWN:
				vec = { 0.f, 1.f, 0.f };
				break;
			case LEFT_DOWN:
				vec = { -1.f, 1.f, 0.f };
				break;
			default:
				vec = { 0.f, 0.f, 0.f };
				// this->vec = {{ 0.f, 0.f }};
				break;
			}
		}

	};
}
}

#endif // GAMEPLAY_CHARACTER_STATE_HPP
