
#ifndef GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP
#define GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP

#include "core/maths/Vector.hpp"

#include "engine/Command.hpp"
#include "engine/Entity.hpp"
#include "engine/physics/physics.hpp"

#include "gameplay/debug.hpp"

#include "utility/any.hpp"

namespace gameplay
{
	namespace component
	{
		struct OverviewCamera
		{
			engine::Entity camera;

			int move_left = 0;
			int move_right = 0;
			int move_down = 0;
			int move_up = 0;

			OverviewCamera(engine::Entity camera) : camera(camera) {}

			void translate(engine::Command command, utility::any && data)
			{
				switch (command)
				{
				case engine::command::MOVE_LEFT_DOWN:
					debug_assert(!data.has_value());
					move_left++;
					break;
				case engine::command::MOVE_LEFT_UP:
					debug_assert(!data.has_value());
					move_left--;
					break;
				case engine::command::MOVE_RIGHT_DOWN:
					debug_assert(!data.has_value());
					move_right++;
					break;
				case engine::command::MOVE_RIGHT_UP:
					debug_assert(!data.has_value());
					move_right--;
					break;
				case engine::command::MOVE_DOWN_DOWN:
					debug_assert(!data.has_value());
					move_down++;
					break;
				case engine::command::MOVE_DOWN_UP:
					debug_assert(!data.has_value());
					move_down--;
					break;
				case engine::command::MOVE_UP_DOWN:
					debug_assert(!data.has_value());
					move_up++;
					break;
				case engine::command::MOVE_UP_UP:
					debug_assert(!data.has_value());
					move_up--;
					break;
				default:
					debug_printline(gameplay::gameplay_channel, "OverviewCamera: Unknown command: ", command);
				}
			}

			void update()
			{
				if (move_left > 0)
					engine::physics::camera::update(
						camera,
						core::maths::Vector3f{-1.f, 0.f, 0.f});
				if (move_right > 0)
					engine::physics::camera::update(
						camera,
						core::maths::Vector3f{1.f, 0.f, 0.f});
				if (move_up > 0)
					engine::physics::camera::update(
						camera,
						core::maths::Vector3f{0.f, 0.f, -1.f});
				if (move_down > 0)
					engine::physics::camera::update(
						camera,
						core::maths::Vector3f{0.f, 0.f, 1.f});
			}
		};
	}
}

#endif /* GAMEPLAY_COMPONENT_OVERVIEWCAMERA_HPP */
