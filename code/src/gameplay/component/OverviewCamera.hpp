
#ifndef GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP
#define GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP

#include "core/maths/Vector.hpp"

#include "engine/Entity.hpp"
#include "engine/physics/physics.hpp"

#include "gameplay/commands.hpp"
#include "gameplay/debug.hpp"

#include "utility/any.hpp"

namespace gameplay
{
	namespace component
	{
		struct OverviewCamera
		{
			engine::physics::simulation * simulation;

			engine::Entity camera;

			float move_left = 0.f;
			float move_right = 0.f;
			float move_down = 0.f;
			float move_up = 0.f;

			OverviewCamera(engine::physics::simulation & simulation, engine::Entity camera) : simulation(&simulation), camera(camera) {}

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
				default:
					debug_printline(gameplay::gameplay_channel, "OverviewCamera: Unknown command: ", command);
				}
			}

			void update()
			{
				const float movement_speed = 1.f;
				if (move_left)
					engine::physics::camera::update(
						*simulation,
						camera,
						core::maths::Vector3f{-(move_left * movement_speed), 0.f, 0.f});
				if (move_right)
					engine::physics::camera::update(
						*simulation,
						camera,
						core::maths::Vector3f{move_right * movement_speed, 0.f, 0.f});
				if (move_up)
					engine::physics::camera::update(
						*simulation,
						camera,
						core::maths::Vector3f{0.f, 0.f, -(move_up * movement_speed)});
				if (move_down)
					engine::physics::camera::update(
						*simulation,
						camera,
						core::maths::Vector3f{0.f, 0.f, move_down * movement_speed});
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP */
