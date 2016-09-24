
#include "characters.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>

#include <engine/graphics/viewer.hpp>
#include <engine/hid/input.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/CharacterState.hpp>

using core::maths::Vector2f;
using core::maths::Vector3f;

namespace
{
	using ::gameplay::characters::MovementState;
	using ::gameplay::characters::CharacterState;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, gameplay::characters::Command>, 100> queue_commands;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueCreate;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueRemove;
	core::container::CircleQueueSRMW<std::pair<engine::Entity, Vector3f>, 100> queueGrounded;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueFalling;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::Entity>, 10> queue_add_cameras;

	core::container::Collection
	<
		engine::Entity,
		101,
		std::array<CharacterState, 50>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	components;

	struct clear_ground_state
	{
		void operator () (CharacterState & x)
		{
			x.clrGrounded();
		}
		template <typename X>
		void operator () (X & x) {}
	};
	struct update_ground_state
	{
		const Vector3f & normal;

		update_ground_state(const Vector3f & normal) : normal(normal) {}

		void operator () (CharacterState & x)
		{
			x.setGrounded(normal);
		}
		template <typename X>
		void operator () (X & x) {}
	};

	struct Camera
	{
		engine::Entity camera;
		engine::Entity target;

		Camera(engine::Entity camera, engine::Entity target) :
			camera(camera), target(target)
		{}

		void update()
		{
			Point pos;
			Vector vec;
			float angle;
			engine::physics::query::positionOf(target, pos, vec, angle);

			vec[0] *= 0.25f;
			vec[1] *= 0.25f;
			vec[2] *= 0.25f;

			const core::maths::Vector3f goal{pos[0] + vec[0], pos[1] + vec[1], 5.f};
			static core::maths::Vector3f current{0.f, 0.f, 20.f};
			const auto delta = goal - current;

			current += delta * .1f;
			engine::graphics::viewer::update(camera, engine::graphics::viewer::translation(current));

			const auto qw = std::cos(angle / 2.f);
			const auto qx = 0.f;
			const auto qy = 0.f;
			const auto qz = 1.f * std::sin(angle / 2.f);
			engine::graphics::viewer::update(camera, engine::graphics::viewer::rotation(core::maths::Quaternionf{qw, qx, qy, qz}));

			engine::graphics::viewer::set_active_3d(camera); // this should not be done every time
		}
	};

	core::container::Collection
	<
		engine::Entity,
		11,
		std::array<Camera, 5>,
		// clang errors on collections with only one array, so here is
		// a dummy array to satisfy it
		std::array<int, 1>
	>
	more_components;
}

namespace gameplay
{
namespace characters
{
	void update()
	{
		{
			engine::Entity id;

			// create
			while (queueCreate.try_pop(id))
			{
				components.emplace<CharacterState>(id);
			}

			// remove
			while (queueRemove.try_pop(id))
			{
				components.remove(id);
			}

			// commands
			{
				std::pair<engine::Entity, gameplay::characters::Command> command;
				while (queue_commands.try_pop(command))
				{
					components.update(command.first, command.second);
				}
			}

			std::pair<engine::Entity, Vector3f> data;

			// update grounded state
			while (queueGrounded.try_pop(data))
			{
				components.call(data.first, update_ground_state{data.second});
			}

			// update falling state
			while (queueFalling.try_pop(id))
			{
				components.call(id, clear_ground_state{});
			}
		}
		{
			std::pair<engine::Entity, engine::Entity> camera;
			while (queue_add_cameras.try_pop(camera))
			{
				debug_printline(0xffffffff, "Camera ", camera.first, " targets entity ", camera.second);
				more_components.emplace<Camera>(camera.first, camera.first, camera.second);
			}
		}

		// update the characters
		for (auto & component : components.get<CharacterState>())
		{
			component.update();
		}

		for (auto & component : components.get<CharacterState>())
		{
			auto res = engine::physics::update(components.get_key(component),
			                                   engine::physics::MoveData(component.movement(),
			                                                             component.fallVel));
			component.fallVel = res.velY;
		}

		// update the cameras
		for (auto & component : more_components.get<Camera>())
		{
			component.update();
		}
	}

	void create(const engine::Entity id)
	{
		const auto res = queueCreate.try_push(id);
		debug_assert(res);
	}

	void remove(const engine::Entity id)
	{
		const auto res = queueRemove.try_push(id);
		debug_assert(res);
	}

	void post_command(engine::Entity id, Command command)
	{
		const auto res = queue_commands.try_emplace(id, command);
		debug_assert(res);
	}

	void postGrounded(const engine::Entity id, const core::maths::Vector3f normal)
	{
		const auto res = queueGrounded.try_emplace(id, normal);
		debug_assert(res);
	}

	void postFalling(const engine::Entity id)
	{
		const auto res = queueFalling.try_push(id);
		debug_assert(res);
	}

	void post_add_camera(engine::Entity id, engine::Entity target)
	{
		const auto res = queue_add_cameras.try_emplace(id, target);
		debug_assert(res);
	}
}
}
