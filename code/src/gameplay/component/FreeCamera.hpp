
#ifndef GAMEPLAY_COMPONENT_FREECAMERA_HPP
#define GAMEPLAY_COMPONENT_FREECAMERA_HPP

#include "core/maths/Quaternion.hpp"
#include "core/maths/util.hpp"

#include "engine/Command.hpp"
#include "engine/Entity.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/physics/physics.hpp"

#include "gameplay/debug.hpp"

#include "utility/any.hpp"

#include <cmath>

namespace gameplay
{
	namespace component
	{
		struct FreeCamera
		{
			engine::Entity camera;

			bool move_left = false;
			bool move_right = false;
			bool move_down = false;
			bool move_up = false;
			bool turn_left = false;
			bool turn_right = false;
			bool turn_down = false;
			bool turn_up = false;
			bool roll_left = false;
			bool roll_right = false;
			bool elevate_down = false;
			bool elevate_up = false;

			core::maths::Quaternionf rotation = {1.f, 0.f, 0.f, 0.f};

			FreeCamera(engine::Entity camera) : camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case engine::command::MOVE_LEFT_DOWN:
					debug_assert(!data.has_value());
					move_left = true;
					break;
				case engine::command::MOVE_LEFT_UP:
					debug_assert(!data.has_value());
					move_left = false;
					break;
				case engine::command::MOVE_RIGHT_DOWN:
					debug_assert(!data.has_value());
					move_right = true;
					break;
				case engine::command::MOVE_RIGHT_UP:
					debug_assert(!data.has_value());
					move_right = false;
					break;
				case engine::command::MOVE_DOWN_DOWN:
					debug_assert(!data.has_value());
					move_down = true;
					break;
				case engine::command::MOVE_DOWN_UP:
					debug_assert(!data.has_value());
					move_down = false;
					break;
				case engine::command::MOVE_UP_DOWN:
					debug_assert(!data.has_value());
					move_up = true;
					break;
				case engine::command::MOVE_UP_UP:
					debug_assert(!data.has_value());
					move_up = false;
					break;
				case engine::command::TURN_LEFT_DOWN:
					debug_assert(!data.has_value());
					turn_left = true;
					break;
				case engine::command::TURN_LEFT_UP:
					debug_assert(!data.has_value());
					turn_left = false;
					break;
				case engine::command::TURN_RIGHT_DOWN:
					debug_assert(!data.has_value());
					turn_right = true;
					break;
				case engine::command::TURN_RIGHT_UP:
					debug_assert(!data.has_value());
					turn_right = false;
					break;
				case engine::command::TURN_DOWN_DOWN:
					debug_assert(!data.has_value());
					turn_down = true;
					break;
				case engine::command::TURN_DOWN_UP:
					debug_assert(!data.has_value());
					turn_down = false;
					break;
				case engine::command::TURN_UP_DOWN:
					debug_assert(!data.has_value());
					turn_up = true;
					break;
				case engine::command::TURN_UP_UP:
					debug_assert(!data.has_value());
					turn_up = false;
					break;
				case engine::command::ROLL_LEFT_DOWN:
					debug_assert(!data.has_value());
					roll_left = true;
					break;
				case engine::command::ROLL_LEFT_UP:
					debug_assert(!data.has_value());
					roll_left = false;
					break;
				case engine::command::ROLL_RIGHT_DOWN:
					debug_assert(!data.has_value());
					roll_right = true;
					break;
				case engine::command::ROLL_RIGHT_UP:
					debug_assert(!data.has_value());
					roll_right = false;
					break;
				case engine::command::ELEVATE_DOWN_DOWN:
					debug_assert(!data.has_value());
					elevate_down = true;
					break;
				case engine::command::ELEVATE_DOWN_UP:
					debug_assert(!data.has_value());
					elevate_down = false;
					break;
				case engine::command::ELEVATE_UP_DOWN:
					debug_assert(!data.has_value());
					elevate_up = true;
					break;
				case engine::command::ELEVATE_UP_UP:
					debug_assert(!data.has_value());
					elevate_up = false;
					break;
				default:
					debug_printline(gameplay::gameplay_channel, "FreeCamera: Unknown command: ", command);
				}
			}

			void update()
			{
				const float rotation_speed = make_radian(core::maths::degreef{1.f/2.f}).get();
				if (turn_left)
					rotation *= core::maths::Quaternionf{std::cos(rotation_speed), 0.f, std::sin(rotation_speed), 0.f};
				if (turn_right)
					rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), 0.f, std::sin(-rotation_speed), 0.f};
				if (turn_up)
					rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), std::sin(-rotation_speed), 0.f, 0.f};
				if (turn_down)
					rotation *= core::maths::Quaternionf{std::cos(rotation_speed), std::sin(rotation_speed), 0.f, 0.f};
				if (roll_left)
					rotation *= core::maths::Quaternionf{std::cos(rotation_speed), 0.f, 0.f, std::sin(rotation_speed)};
				if (roll_right)
					rotation *= core::maths::Quaternionf{std::cos(-rotation_speed), 0.f, 0.f, std::sin(-rotation_speed)};

				const float movement_speed = .5f;
				if (move_left)
					engine::physics::camera::update(
						camera,
						-rotation.axis_x() * movement_speed);
				if (move_right)
					engine::physics::camera::update(
						camera,
						rotation.axis_x() * movement_speed);
				if (move_up)
					engine::physics::camera::update(
						camera,
						-rotation.axis_z() * movement_speed);
				if (move_down)
					engine::physics::camera::update(
						camera,
						rotation.axis_z() * movement_speed);
				if (elevate_down)
					engine::physics::camera::update(
						camera,
						-rotation.axis_y() * movement_speed);
				if (elevate_up)
					engine::physics::camera::update(
						camera,
						rotation.axis_y() * movement_speed);

				engine::graphics::viewer::post_update_camera(camera, engine::graphics::viewer::rotation{rotation});
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_FREECAMERA_HPP */
