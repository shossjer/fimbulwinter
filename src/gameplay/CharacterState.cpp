
#include "CharacterState.hpp"

namespace gameplay
{
namespace characters
{
	void CharacterState::updateChillin(Command command)
	{
		switch (command)
		{
		case Command::JUMP:

			setFlag(BIT_JUMPING);
			engine::animation::update(this->me, ::engine::animation::action{ "jump-00", false });
			printf("action: Jumping\n");
			this->pActionFunc = &CharacterState::updateJumping;
			break;

		case Command::PHYSICS_FALLING:

			engine::animation::update(this->me, ::engine::animation::action{ "fall-00", true });
			printf("action: Falling\n");
			this->pActionFunc = &CharacterState::updateFalling;
			break;

		case Command::GO_LEFT:

			setFlag(BIT_MOVE_LEFT);
			engine::physics::post_set_heading(this->me, core::maths::degreef{ 90.f });
			engine::animation::update(this->me, ::engine::animation::action{ "sprint-00", true });
			printf("action: Movin\n");
			this->pActionFunc = &CharacterState::updateMovin;
			break;
		case Command::GO_RIGHT:

			setFlag(BIT_MOVE_RIGHT);
			engine::physics::post_set_heading(this->me, core::maths::degreef{ 270.f });
			engine::animation::update(this->me, ::engine::animation::action{ "sprint-00", true });
			printf("action: Movin\n");
			this->pActionFunc = &CharacterState::updateMovin;
			break;

		default:
			// do nothing
			break;
		}
	}

	void CharacterState::updateMovin(const Command command)
	{
		switch (command)
		{
		case Command::JUMP:

			setFlag(BIT_JUMPING);
			engine::animation::update(this->me, ::engine::animation::action{ "jump-00", false });
			printf("action: Jumping\n");
			this->pActionFunc = &CharacterState::updateJumping;
			break;

		case Command::PHYSICS_FALLING:

			engine::animation::update(this->me, ::engine::animation::action{ "fall-00", true });
			printf("action: Falling\n");
			this->pActionFunc = &CharacterState::updateFalling;
			break;

		case Command::GO_LEFT:

			setFlag(BIT_MOVE_LEFT);
			engine::physics::post_set_heading(this->me, core::maths::degreef{ 90.f });
			engine::animation::update(this->me, ::engine::animation::action{ "sprint-00", true });
			break;
		case Command::GO_RIGHT:

			setFlag(BIT_MOVE_RIGHT);
			engine::physics::post_set_heading(this->me, core::maths::degreef{ 270.f });
			engine::animation::update(this->me, ::engine::animation::action{ "sprint-00", true });
			break;

		case Command::STOP_ITS_HAMMER_TIME:

			clrFlag(BIT_MOVE_LEFT);
			clrFlag(BIT_MOVE_RIGHT);
			engine::animation::update(this->me, ::engine::animation::action{ "stand-00", true });
			printf("action: Chillin\n");
			this->pActionFunc = &CharacterState::updateChillin;
			break;
		default:
			// do nothing
			break;
		}
	}

	void CharacterState::updateJumping(const Command command)
	{
		switch (command)
		{
		// temp I hope
		case Command::GO_LEFT:
			setFlag(BIT_MOVE_LEFT);
			break;
		case Command::GO_RIGHT:
			setFlag(BIT_MOVE_RIGHT);
			break;
		case Command::STOP_ITS_HAMMER_TIME:
			clrFlag(BIT_MOVE_LEFT);
			clrFlag(BIT_MOVE_RIGHT);
			break;

		case Command::PHYSICS_GROUNDED:

			engine::animation::update(this->me, ::engine::animation::action{ "land-00", false });
			printf("action: Chillin\n");
			this->pActionFunc = &CharacterState::updateChillin;
			break;

		case Command::ANIMATION_FINISHED:

			engine::animation::update(this->me, ::engine::animation::action{ "fall-00", true });
			printf("action: Falling\n");
			this->pActionFunc = &CharacterState::updateFalling;
			break;

		default:
			// do nothing
			break;
		}
	}

	void CharacterState::updateFalling(const Command command)
	{
		switch (command)
		{
		// temp I hope
		case Command::GO_LEFT:
			setFlag(BIT_MOVE_LEFT);
			break;
		case Command::GO_RIGHT:
			setFlag(BIT_MOVE_RIGHT);
			break;
		case Command::STOP_ITS_HAMMER_TIME:
			clrFlag(BIT_MOVE_LEFT);
			clrFlag(BIT_MOVE_RIGHT);
			break;

		case Command::PHYSICS_GROUNDED:

			engine::animation::update(this->me, ::engine::animation::action{ "land-00", false });
			printf("action: Landing\n");
			this->pActionFunc = &CharacterState::updateLanding;
			break;

		default:
			// do nothing
			break;
		}
	}

	void CharacterState::updateLanding(const Command command)
	{
		switch (command)
		{
		// temp I hope
		case Command::GO_LEFT:
			setFlag(BIT_MOVE_LEFT);
			break;
		case Command::GO_RIGHT:
			setFlag(BIT_MOVE_RIGHT);
			break;
		case Command::STOP_ITS_HAMMER_TIME:
			clrFlag(BIT_MOVE_LEFT);
			clrFlag(BIT_MOVE_RIGHT);
			break;

		case Command::ANIMATION_FINISHED:

			if (hasFlag(BIT_MOVE_LEFT))
			{
				updateChillin(Command::GO_LEFT);
			}
			else
			if (hasFlag(BIT_MOVE_RIGHT))
			{
				updateChillin(Command::GO_RIGHT);
			}
			else
			{
				engine::animation::update(this->me, ::engine::animation::action{ "stand-00", true });
				printf("action: Chillin\n");
				this->pActionFunc = &CharacterState::updateChillin;
			}
			break;

		case Command::PHYSICS_FALLING:

			engine::animation::update(this->me, ::engine::animation::action{ "fall-00", true });
			printf("action: Falling\n");
			this->pActionFunc = &CharacterState::updateFalling;
			break;

		default:
			// do nothing
			break;
		}
	}
}
}