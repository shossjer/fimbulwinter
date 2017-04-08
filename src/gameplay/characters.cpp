
#include "characters.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>

#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>
#include <engine/hid/input.hpp>
#include <engine/physics/Callback.hpp>
#include <engine/physics/physics.hpp>

#include <gameplay/CharacterState.hpp>

using Vector2f = core::maths::Vector2f;
using Vector3f = core::maths::Vector3f;
using Matrix4x4f = core::maths::Matrix4x4f;
using Quaternionf = core::maths::Quaternionf;

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
	core::container::CircleQueueSRMW<::gameplay::asset::turret_t, 100> queueTurrets;
	core::container::CircleQueueSRMW<::gameplay::asset::droid_t, 100> queueDroids;

	core::container::CircleQueueSRMW<collision_t, 100> queueCollisionsFound;
	core::container::CircleQueueSRMW<collision_t, 100> queueCollisionsLeft;

	core::container::CircleQueueSRMW<std::pair<engine::Entity, engine::Entity>, 10> queue_add_cameras;

	core::container::Collection
	<
		engine::Entity,
		101,
		std::array<::gameplay::characters::trigger_t, 20>,
		std::array<::gameplay::asset::droid_t, 20>,
		std::array<::gameplay::asset::turret_t, 20>
	>
	components;

	struct procedure_translate
	{
		const ::engine::transform_t & transform;

		void operator () (::gameplay::asset::asset_t & x)
		{
			x.transform.pos = transform.pos;
			x.transform.quat = transform.quat;
		}

		// object was not found callback
		void operator () ()
		{
		}

		template <typename X>
		void operator () (X & x)
		{
		}
	};

	struct extract_translation
	{
		// how do I check base type instead? asset::asset_t
		engine::transform_t * operator () (::gameplay::asset::droid_t & x)
		{
			return & x.transform;
		}

		// object was not found callback
		engine::transform_t * operator () ()
		{
			return nullptr;
		}

		template <typename X>
		engine::transform_t * operator () (X & x)
		{
			return nullptr;
		}
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
			engine::transform_t *const transform = components.try_call(target, extract_translation());

			if (transform==nullptr)
			{
				return;
			}

			//vec *= 0.25f;
			const Vector3f goal = transform->pos + core::maths::Vector3f {0.f, 0.f, 10.f};

			static core::maths::Vector3f current {0.f, 0.f, 50.f};
			const auto delta = goal - current;

			current += delta * .1f;
			engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::translation(current));

			//const auto qw = std::cos(angle/2.f);
			//const auto qx = 0.f;
			//const auto qy = 0.f;
			//const auto qz = 1.f * std::sin(angle/2.f);

			// using the rotation in the camera made me very dizzy...
			//engine::graphics::viewer::update(camera, engine::graphics::viewer::rotation(rot));
			engine::graphics::viewer::update(
					camera,
					engine::graphics::viewer::rotation(Quaternionf {1.f, 0.f, 0.f, 0.f}));

			engine::graphics::viewer::set_active_3d(camera); // this should not be done every time
		}
	};

	core::container::Collection
	<
		engine::Entity,
		11,
		std::array<Camera, 5>,
		std::array<CharacterState, 5>
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
			// must be a valid type of target
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				break;

				default:
				return;
			}

			// add on queue
			queueCollisionsFound.try_emplace(data);

			// debug
			switch (data.behaviours[0])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision found (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				debug_printline(0xffffffff, "Character collision found");
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
			// must be a valid type of target
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				break;

				default:
				return;
			}

			// add on queue
			queueCollisionsLeft.try_emplace(data);

			// debug
			switch (data.behaviours[0])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision lost (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				debug_printline(0xffffffff, "Character collision lost");
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
			// must be a valid type of target
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				break;

				default:
				return;
			}

			// add on queue
			queueCollisionsFound.try_emplace(data);

			// debug
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision found with: Trigger (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				debug_printline(0xffffffff, "Trigger collision found with: Character");
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
			// must be a valid type of target
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				break;

				default:
				return;
			}
			// add on queue
			queueCollisionsLeft.try_emplace(data);

			// debug
			switch (data.behaviours[1])
			{
				case ::engine::physics::ActorData::Behaviour::TRIGGER:
				debug_printline(0xffffffff, "Trigger collision lost with: Trigger (should not happen)");
				break;
				case ::engine::physics::ActorData::Behaviour::CHARACTER:
				debug_printline(0xffffffff, "Trigger collision lost with: Character");
				break;
				case ::engine::physics::ActorData::Behaviour::OBSTACLE:
				debug_printline(0xffffffff, "Trigger collision lost with: Obstacle");
				break;
				case ::engine::physics::ActorData::Behaviour::DEFAULT:
				debug_printline(0xffffffff, "Trigger collision lost with: Default");
				break;
			}
		}

		void postTransformation(
				const ::engine::Entity id,
				const ::engine::transform_t & transform)
				const override
		{
			engine::graphics::data::ModelviewMatrix modelviewMatrix = {
					make_translation_matrix(transform.pos) *
					make_matrix(transform.quat)
			};

			// update the entity instance
			components.try_call(id, procedure_translate{ transform });

			// update renderer
			engine::graphics::renderer::update(id, modelviewMatrix);
		}

	} physicsCallback;

	void setup()
	{
		::engine::physics::subscribe(physicsCallback);
	}

	void update()
	{
		static float timeFrame = 0;
		{
			engine::Entity id;

			// create
			while (queueCreate.try_pop(id))
			{
				more_components.emplace<CharacterState>(id, id);
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
					more_components.update(command.first, command.second);
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
			asset::turret_t turret;
			while (queueTurrets.try_pop(turret))
			{
				turret.timestamp = timeFrame + 3.f;
				components.emplace<asset::turret_t>(turret.id, turret);
			}
		}
		{
			asset::droid_t droid;
			while (queueDroids.try_pop(droid))
			{
				components.emplace<asset::droid_t>(droid.id, droid);
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
		for (CharacterState & component : more_components.get<CharacterState>())
		{
			engine::physics::post_update_movement(
				component.id,
				engine::physics::movement_data {engine::physics::movement_data::Type::ACCELERATION, component.movement} );
		}

		// update turrets!
		for (auto & turret : components.get<asset::turret_t>())
		{
			if (turret.timestamp > timeFrame) continue;

			turret.timestamp = timeFrame + 1.f;

			const Vector3f pos = turret.transform.pos + turret.projectile.pos;

			const auto id = engine::Entity::create();
			const float radie = 0.1f;

			// Fire!
			{
				std::vector<engine::physics::ShapeData> shapes;
				shapes.push_back(engine::physics::ShapeData {
					engine::physics::ShapeData::Type::SPHERE,
					engine::physics::Material::STONE,
					0.5f,
					Vector3f{0.f, 0.f, 0.f},
					turret.projectile.quat,
					engine::physics::ShapeData::Geometry {engine::physics::ShapeData::Geometry::Sphere {radie}}});

				engine::physics::ActorData data {
					engine::physics::ActorData::Type::PROJECTILE,
					engine::physics::ActorData::Behaviour::PROJECTILE,
					engine::transform_t{pos, turret.transform.quat},
					shapes};

				engine::physics::post_create(id, data);
				engine::physics::post_update_movement(
					id,
					engine::physics::movement_data {
						engine::physics::movement_data::Type::IMPULSE,
						Vector3f{-200.f, 0.f, 0.f}});
			}
			{
				engine::graphics::data::CuboidC data = {
					make_translation_matrix(pos),
					radie*2,
					radie*2,
					radie*2,
					0xffffffff
				};
				engine::graphics::renderer::add(id, data);
			}
		}

		// update the cameras
		for (auto & component : more_components.get<Camera>())
		{
			component.update();
		}

		timeFrame += 1.f/50.f;
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

	void post_add(asset::turret_t & turret)
	{
		const auto res = queueTurrets.try_emplace(turret);
		debug_assert(res);
	}

	void post_add(asset::droid_t & droid)
	{
		const auto res = queueDroids.try_emplace(droid);
		debug_assert(res);
	}

	void post_animation_finish(engine::Entity id)
	{
		const auto res = queueAnimationFinished.try_emplace(id);
		debug_assert(res);
	}
}
}
