
#ifndef GAMEPLAY_COMPONENT_FREECAMERA_HPP
#define GAMEPLAY_COMPONENT_FREECAMERA_HPP

#include "core/maths/Quaternion.hpp"
#include "core/maths/util.hpp"

#include "engine/Entity.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/physics/physics.hpp"

#include "gameplay/commands.hpp"
#include "gameplay/debug.hpp"

#include "utility/any.hpp"

#include <cmath>

namespace gameplay
{
	namespace component
	{
		struct FreeCamera
		{
			engine::graphics::viewer * viewer;
			engine::physics::simulation * simulation;

			engine::Entity camera;

			float move_left = 0.f;
			float move_right = 0.f;
			float move_down = 0.f;
			float move_up = 0.f;
			float turn_left = 0.f;
			float turn_right = 0.f;
			float turn_down = 0.f;
			float turn_up = 0.f;
			float roll_left = 0.f;
			float roll_right = 0.f;
			float elevate_down = 0.f;
			float elevate_up = 0.f;

			core::maths::Quaternionf rotation = {1.f, 0.f, 0.f, 0.f};

			FreeCamera(engine::graphics::viewer & viewer, engine::physics::simulation & simulation, engine::Entity camera) : viewer(&viewer), simulation(&simulation), camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case gameplay::command::MOVE_LEFT:
					debug_assert(data.has_value());
					move_left = utility::any_cast<float>(data);
					break;
				case gameplay::command::MOVE_RIGHT:
					debug_assert(data.has_value());
					move_right = utility::any_cast<float>(data);
					break;
				case gameplay::command::MOVE_DOWN:
					debug_assert(data.has_value());
					move_down = utility::any_cast<float>(data);
					break;
				case gameplay::command::MOVE_UP:
					debug_assert(data.has_value());
					move_up = utility::any_cast<float>(data);
					break;
				case gameplay::command::TURN_LEFT:
					debug_assert(data.has_value());
					turn_left = utility::any_cast<float>(data);
					break;
				case gameplay::command::TURN_RIGHT:
					debug_assert(data.has_value());
					turn_right = utility::any_cast<float>(data);
					break;
				case gameplay::command::TURN_DOWN:
					debug_assert(data.has_value());
					turn_down = utility::any_cast<float>(data);
					break;
				case gameplay::command::TURN_UP:
					debug_assert(data.has_value());
					turn_up = utility::any_cast<float>(data);
					break;
				case gameplay::command::ROLL_LEFT:
					debug_assert(data.has_value());
					roll_left = utility::any_cast<float>(data);
					break;
				case gameplay::command::ROLL_RIGHT:
					debug_assert(data.has_value());
					roll_right = utility::any_cast<float>(data);
					break;
				case gameplay::command::ELEVATE_DOWN:
					debug_assert(data.has_value());
					elevate_down = utility::any_cast<float>(data);
					break;
				case gameplay::command::ELEVATE_UP:
					debug_assert(data.has_value());
					elevate_up = utility::any_cast<float>(data);
					break;
				default:
					debug_printline(gameplay::gameplay_channel, "FreeCamera: Unknown command: ", command);
				}
			}

			void update()
			{
				const float rotation_speed = make_radian(core::maths::degreef{1.f/2.f}).get();
				if (turn_left != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(turn_left * rotation_speed), 0.f, std::sin(turn_left * rotation_speed), 0.f};
				if (turn_right != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(-(turn_right * rotation_speed)), 0.f, std::sin(-(turn_right * rotation_speed)), 0.f};
				if (turn_up != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(-(turn_up * rotation_speed)), std::sin(-(turn_up * rotation_speed)), 0.f, 0.f};
				if (turn_down != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(turn_down * rotation_speed), std::sin(turn_down * rotation_speed), 0.f, 0.f};
				if (roll_left != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(roll_left * rotation_speed), 0.f, 0.f, std::sin(roll_left * rotation_speed)};
				if (roll_right != 0.f)
					rotation *= core::maths::Quaternionf{std::cos(-(roll_right * rotation_speed)), 0.f, 0.f, std::sin(-(roll_right * rotation_speed))};

				const float movement_speed = .5f;
				if (move_left != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						-rotation.axis_x() * (move_left * movement_speed));
				if (move_right != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						rotation.axis_x() * (move_right * movement_speed));
				if (move_up != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						-rotation.axis_z() * (move_up * movement_speed));
				if (move_down != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						rotation.axis_z() * (move_down * movement_speed));
				if (elevate_down != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						-rotation.axis_y() * (elevate_down * movement_speed));
				if (elevate_up != 0.f)
					engine::physics::camera::update(
						*simulation,
						camera,
						rotation.axis_y() * (elevate_up * movement_speed));

				post_update_camera(*viewer, camera, engine::graphics::viewer::rotation{rotation});
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_FREECAMERA_HPP */
