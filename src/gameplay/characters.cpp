
#include "characters.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>

#include <engine/graphics/viewer.hpp>
#include <engine/hid/input.hpp>
#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/CharacterState.hpp>

using core::maths::Vector2f;
using core::maths::Vector3f;

namespace
{
	using ::gameplay::characters::CharacterState;
	using collision_t = ::engine::physics::Callback::data_t;
	using trigger_t = ::gameplay::characters::trigger_t;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, gameplay::characters::Command>, 100> queue_commands;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueCreate;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueRemove;
	core::container::CircleQueueSRMW<engine::Entity, 100> queueAnimationFinished;

	core::container::CircleQueueSRMW<::gameplay::characters::trigger_t, 100> queueTriggers;
	core::container::CircleQueueSRMW<::gameplay::characters::turret_t, 100> queueTurrets;

	core::container::CircleQueueSRMW<collision_t, 100> queueCollisionsFound;
	core::container::CircleQueueSRMW<collision_t, 100> queueCollisionsLeft;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::Entity>, 10> queue_add_cameras;

	core::container::Collection
	<
		engine::Entity,
		101,
		std::array<CharacterState, 20>,
		std::array<::gameplay::characters::trigger_t, 20>,
		std::array<::gameplay::characters::turret_t, 20>
		//// clang errors on collections with only one array, so here is
		//// a dummy array to satisfy it
		//std::array<int, 1>
	>
	components;

	struct clear_ground_state
	{
		void operator () (CharacterState & x)
		{
			x.clrGrounded();
		//	x = gameplay::characters::Command::PHYSICS_FALLING;
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
		//	x = gameplay::characters::Command::PHYSICS_GROUNDED;
		}
		template <typename X>
		void operator () (X & x) {}
	};

	struct animation_finished
	{
		void operator () (CharacterState & x)
		{
		//	x = gameplay::characters::Command::ANIMATION_FINISHED;
		}
		template <typename X>
		void operator () (X & x) {}
	};

	void send(const std::string & action, const bool repeat, const std::vector<::engine::Entity> & targets)
	{
		for (const auto & target : targets)
		{
			::engine::animation::update(target, ::engine::animation::action {action, repeat});
		}
	}

	struct action_collision_found
	{
		void operator() (trigger_t & trigger)
		{
			switch (trigger.type)
			{
				case trigger_t::Type::DOOR:
				{
					debug_printline(0xffffffff, "collision found for door type trigger");
					send("open", false, trigger.targets);
					break;
				}
				default:
					debug_printline(0xffffffff, "collision found for unknown type trigger");
			}
		}
		template <typename X>
		void operator () (X & x)
		{
			debug_printline(0xffffffff, "collision found for unused class");
		}
	};

	struct action_collision_lost
	{
		void operator() (trigger_t & trigger)
		{
			switch (trigger.type)
			{
				case trigger_t::Type::DOOR:
				{
					debug_printline(0xffffffff, "collision lost for door type trigger");
					send("close", false, trigger.targets);
					break;
				}
				default:
					debug_printline(0xffffffff, "collision lost for unknown type trigger");
			}
		}
		template <typename X>
		void operator () (X & x)
		{
			debug_printline(0xffffffff, "collision lost for unused class");
		}
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
			Vector3f pos;
			Quaternionf rot;
			Vector3f vec;

			engine::physics::query_position(target, pos, rot, vec);

			Vector3f goal;

			vec *= 0.25f;
			goal = pos+vec+core::maths::Vector3f {0.f, 0.f, 25.f};

			static core::maths::Vector3f current {0.f, 0.f, 20.f};
			const auto delta = goal-current;

			current += delta * .1f;
			engine::graphics::viewer::update(camera, engine::graphics::viewer::translation(current));

			//const auto qw = std::cos(angle/2.f);
			//const auto qx = 0.f;
			//const auto qy = 0.f;
			//const auto qz = 1.f * std::sin(angle/2.f);

			// using the rotation in the camera made me very dizzy...
			//engine::graphics::viewer::update(camera, engine::graphics::viewer::rotation(rot));
			engine::graphics::viewer::update(camera, engine::graphics::viewer::rotation(Quaternionf {1.f, 0.f, 0.f, 0.f}));

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

namespace engine
{
	namespace physics
	{
		extern void subscribe(const engine::physics::Callback & callback);
	}
}

namespace gameplay
{
namespace characters
{
	class PhysicsCallback : public ::engine::physics::Callback
	{
	public:

		void postContactFound(const data_t & data) const override
		{
			// add on queue
			queueCollisionsFound.try_emplace(data);

			// debug
			switch (data.behaviours[0])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision found (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::PLAYER:
				debug_printline(0xffffffff, "Player collision found");
				break;
				case ::engine::physics::ActorData::Behaviour::OBSTACLE:
				debug_printline(0xffffffff, "Obstacle collision found");
				break;
				case ::engine::physics::ActorData::Behaviour::DEFAULT:
				debug_printline(0xffffffff, "Default collision found (should not happen)");
				break;
			}
		}

		void postContactLost(const data_t & data) const override
		{
			// add on queue
			queueCollisionsLeft.try_emplace(data);

			// debug
			switch (data.behaviours[0])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision lost (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::PLAYER:
				debug_printline(0xffffffff, "Player collision lost");
				break;
				case ::engine::physics::ActorData::Behaviour::OBSTACLE:
				debug_printline(0xffffffff, "Obstacle collision lost");
				break;
				case ::engine::physics::ActorData::Behaviour::DEFAULT:
				debug_printline(0xffffffff, "Default collision lost (should not happen)");
				break;
			}
		}

		void postTriggerFound(const data_t & data) const override
		{
			// add on queue
			queueCollisionsFound.try_emplace(data);

			// debug
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision found with: Trigger (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::PLAYER:
				debug_printline(0xffffffff, "Trigger collision found with: Player");
				break;
				case ::engine::physics::ActorData::Behaviour::OBSTACLE:
				debug_printline(0xffffffff, "Trigger collision found with: Obstacle");
				break;
				case ::engine::physics::ActorData::Behaviour::DEFAULT:
				debug_printline(0xffffffff, "Trigger collision found with: Default");
				break;
			}
		}

		void postTriggerLost(const data_t & data) const override
		{
			// add on queue
			queueCollisionsLeft.try_emplace(data);

			// debug
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision lost with: Trigger (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::PLAYER:
				debug_printline(0xffffffff, "Trigger collision lost with: Player");
				break;
				case ::engine::physics::ActorData::Behaviour::OBSTACLE:
				debug_printline(0xffffffff, "Trigger collision lost with: Obstacle");
				break;
				case ::engine::physics::ActorData::Behaviour::DEFAULT:
				debug_printline(0xffffffff, "Trigger collision lost with: Default");
				break;
			}
		}
	} physicsCallback;

	void setup()
	{
		::engine::physics::subscribe(physicsCallback);
	}

	void update()
	{
		{
			engine::Entity id;

			// create
			while (queueCreate.try_pop(id))
			{
				components.emplace<CharacterState>(id, id);
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

			// animation finished
			while (queueAnimationFinished.try_pop(id))
			{
				components.call(id, animation_finished());
			}
		}
		{
			// create
			trigger_t trigger;
			while (queueTriggers.try_pop(trigger))
			{
				components.emplace<trigger_t>(trigger.id, trigger);
			}
		}
		{
			turret_t turret;
			while (queueTurrets.try_pop(turret))
			{
				components.emplace<turret_t>(turret.id, turret);
			}
		}
		{
			// collision callback
			collision_t collision;
			while (queueCollisionsFound.try_pop(collision))
			{
				// find trigger and change its door... totally not hardcoded at all
				components.call(collision.ids[0], action_collision_found());
			}
			while (queueCollisionsLeft.try_pop(collision))
			{
				// find trigger and change its door... totally not hardcoded at all
				components.call(collision.ids[0], action_collision_lost());
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

		// update movement of player
		for (CharacterState & component : components.get<CharacterState>())
		{
			engine::physics::post_update_movement(
				component.id,
				engine::physics::movement_data {engine::physics::movement_data::Type::ACCELERATION, component.movement} );
		}

		// update turrets!
		for (auto & turret:components.get<turret_t>())
		{

		}

		// for (auto & component : components.get<CharacterState>())
		// {
		// 	auto res = engine::physics::update(components.get_key(component),
		// 	                                   engine::physics::MoveData(component.movement(),
		// 	                                                             component.fallVel));
		// 	component.fallVel = res.velY;
		// }

		// update the cameras
		for (auto & component : more_components.get<Camera>())
		{
			component.update();
		}
	}

	void post_create_player(const engine::Entity id)
	{
		const auto res = queueCreate.try_push(id);
		debug_assert(res);
	}

	void post_remove_player(const engine::Entity id)
	{
		const auto res = queueRemove.try_push(id);
		debug_assert(res);
	}

	void post_command(engine::Entity id, Command command)
	{
		const auto res = queue_commands.try_emplace(id, command);
		debug_assert(res);
	}

	void post_add_camera(engine::Entity id, engine::Entity target)
	{
		const auto res = queue_add_cameras.try_emplace(id, target);
		debug_assert(res);
	}

	void post_add_trigger(trigger_t trigger)
	{
		const auto res = queueTriggers.try_emplace(trigger);
		debug_assert(res);
	}

	void post_add_turret(turret_t turret)
	{
		const auto res = queueTurrets.try_emplace(turret);
		debug_assert(res);
	}

	void post_animation_finish(engine::Entity id)
	{
		const auto res = queueAnimationFinished.try_emplace(id);
		debug_assert(res);
	}
}
}
