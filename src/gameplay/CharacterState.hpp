
#ifndef GAMEPLAY_CHARACTER_STATE_HPP
#define GAMEPLAY_CHARACTER_STATE_HPP

#include "characters.hpp"

#include <engine/animation/mixer.hpp>
#include <engine/Entity.hpp>
#include <engine/physics/defines.hpp>
#include <engine/physics/physics.hpp>

#include <core/maths/Vector.hpp>

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

		engine::Entity me;

		using Flag = unsigned int;

		static constexpr Flag BIT_GROUNDED = 0x1;
		static constexpr Flag BIT_JUMPING = 0x2;
		static constexpr Flag BIT_MOVE_LEFT = 0x10;
		static constexpr Flag BIT_MOVE_RIGHT = 0x20;

		Flag flags;

		Vector3f groundNormal;

	public:

		bool isGrounded() const { return (this->flags & BIT_GROUNDED) != 0; }

		const Vector3f & getGroundNormal() const { return this->groundNormal; }

		void (CharacterState::* pActionFunc)(Command);

	public:

		bool hasFlag(const Flag flag) const { return (flag & this->flags) != 0; }

		void setFlag(const Flag flag)
		{
			this->flags |= flag;
		}

		void clrFlag(const Flag flag)
		{
			this->flags &= ~flag;
		}

		void setGrounded(const Vector3f & groundNormal)
		{
			this->groundNormal = groundNormal;
			setFlag(BIT_GROUNDED);
		}

		void clrGrounded()
		{
			clrFlag(BIT_GROUNDED);
		}

	public:

		CharacterState(engine::Entity me) : me(me), flags(0), pActionFunc(&CharacterState::updateChillin)
		{
		}

	public:

		CharacterState & operator = (Command command)
		{
			(this->*pActionFunc)(command);

			return *this;
		}

		void updateChillin(Command command);

		void updateMovin(const Command command);

		void updateJumping(const Command command);

		void updateFalling(const Command command);

		void updateLanding(const Command command);
	};
}
}

#endif // GAMEPLAY_CHARACTER_STATE_HPP
