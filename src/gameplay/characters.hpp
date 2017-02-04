
#ifndef GAMEPLAY_CHARACTERS_HPP
#define GAMEPLAY_CHARACTERS_HPP

#include <engine/Entity.hpp>

#include <core/maths/Vector.hpp>
#include <core/maths/Quaternion.hpp>

namespace gameplay
{
namespace characters
{
	enum struct Command
	{
		LEFT_DOWN,
		LEFT_UP,
		RIGHT_DOWN,
		RIGHT_UP,
		UP_DOWN,
		UP_UP,
		DOWN_DOWN,
		DOWN_UP
	};

	void setup();

	void update();

	void post_create_player(const engine::Entity id);

	void post_remove_player(const engine::Entity id);

	void post_command(engine::Entity id, Command command);

	void post_add_camera(engine::Entity id, engine::Entity target);

	void post_animation_finish(engine::Entity id);

	struct trigger_t
	{
		::engine::Entity id;

		enum class Type
		{
			DOOR
		} type;

		std::vector<::engine::Entity> targets;
	};
	void post_add_trigger(trigger_t trigger);

	struct transform_t
	{
		core::maths::Vector3f pos;
		core::maths::Quaternionf quat;
	};

	struct turret_t
	{
		struct head_t
		{
			::engine::Entity id;
			::engine::Entity jointId;
			transform_t pivot;
		};

		struct barrel_t
		{
			::engine::Entity id;
			::engine::Entity jointId;
			transform_t pivot;
		};

		::engine::Entity id;

		transform_t transform;

		head_t head;

		barrel_t barrel;

		transform_t projectile;

		float timestamp;
	};

	void post_add_turret(turret_t turret);
}
}


#endif /* GAMEPLAY_CHARACTERS_HPP */
