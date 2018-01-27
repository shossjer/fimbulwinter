
#include "physics.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/graphics/renderer.hpp>
#include <engine/graphics/viewer.hpp>

#include <utility/variant.hpp>

#include <limits>

namespace
{
	typedef std::numeric_limits<float> lim;

	engine::physics::camera::Bounds bounds{
		{-lim::max(),-lim::max(),-lim::max() },
		{ lim::max(), lim::max(), lim::max() } };

	struct Camera
	{
		bool bounded;
		Vector3f position;

		void update(Vector3f & movement)
		{
			if (!this->bounded)
			{
				this->position += movement;
			}
			else
			{
				Vector3f np = this->position + movement;

				core::maths::Vector3f::array_type bmin;
				bounds.min.get_aligned(bmin);
				core::maths::Vector3f::array_type bmax;
				bounds.max.get_aligned(bmax);

				core::maths::Vector3f::array_type pos;
				np.get_aligned(pos);

				if (pos[0] < bmin[0]) pos[0] = bmin[0];
				if (pos[1] < bmin[1]) pos[1] = bmin[1];
				if (pos[2] < bmin[2]) pos[2] = bmin[2];

				if (pos[0] > bmax[0]) pos[0] = bmax[0];
				if (pos[1] > bmax[1]) pos[1] = bmax[1];
				if (pos[2] > bmax[2]) pos[2] = bmax[2];

				this->position = Vector3f{ pos[0], pos[1], pos[2] };
			}
		}
	};

	core::container::Collection
	<
		engine::Entity,
		21,
		std::array<Camera, 10>,
		std::array<int, 10>
	>
	components;
}

namespace engine
{
namespace physics
{
namespace camera
{
	// TODO: make thread safe when needed
	void set(Bounds && bounds)
	{
		core::maths::Vector3f::array_type bmin;
		bounds.min.get_aligned(bmin);
		core::maths::Vector3f::array_type bmax;
		bounds.max.get_aligned(bmax);

		debug_assert(bmin[0] <= bmax[0]);
		debug_assert(bmin[1] <= bmax[1]);
		debug_assert(bmin[2] <= bmax[2]);

		::bounds = bounds;

		// snap existing cameras to new bound
		for (Camera & camera : components.get<Camera>())
		{
			update(components.get_key(camera), Vector3f{ 0.f, 0.f, 0.f });
		}
	}

	// TODO: make thread safe when needed
	void add(engine::Entity id, Vector3f position, bool bounded)
	{
		components.emplace<Camera>(id, Camera{ bounded, position });

		// snap new camera to bounds
		update(id, Vector3f{ 0.f, 0.f, 0.f });
	}

	// TODO: make thread safe when needed
	void update(engine::Entity id, Vector3f movement)
	{
		Camera & camera = components.get<Camera>(id);

		camera.update(movement);

		engine::graphics::viewer::post_update_camera(
			id,
			engine::graphics::viewer::translation{ camera.position });
	}
}
}
}

namespace
{
	struct object_t
	{
		core::maths::Vector3f position;
		core::maths::Quaternionf orientation;

		object_t(core::maths::Vector3f position,
		         core::maths::Quaternionf orientation)
			: position(position)
			, orientation(orientation)
		{}
	};

	core::container::Collection
	<
		engine::Entity,
		201,
		std::array<object_t, 100>,
		std::array<object_t, 1>
	>
	objects;

	struct update_movement
	{
		core::maths::Vector3f && movement;

		void operator () (object_t & x)
		{
			x.position += rotate(movement, x.orientation);
		}
	};
	struct update_orientation_movement
	{
		core::maths::Quaternionf && movement;

		void operator () (object_t & x)
		{
			x.orientation *= movement;
		}
	};
	struct update_transform
	{
		engine::transform_t && transform;

		void operator () (object_t & x)
		{
			x.position = std::move(transform.pos);
			x.orientation = std::move(transform.quat);
		}
	};
}

namespace
{
	struct MessageAddObject
	{
		engine::Entity entity;
		engine::transform_t transform;
	};
	struct MessageRemove
	{
		engine::Entity entity;
	};
	struct MessageUpdateMovement
	{
		engine::Entity entity;
		engine::physics::movement_data movement;
	};
	struct MessageUpdateOrientationMovement
	{
		engine::Entity entity;
		engine::physics::orientation_movement movement;
	};
	struct MessageUpdateTransform
	{
		engine::Entity entity;
		engine::transform_t transform;
	};
	using EntityMessage = utility::variant
	<
		MessageAddObject,
		MessageRemove,
		MessageUpdateMovement,
		MessageUpdateOrientationMovement,
		MessageUpdateTransform
	>;

	core::container::CircleQueueSRMW<EntityMessage, 100> queue_entities;
}

namespace engine
{
namespace physics
{
	void create()
	{

	}

	void destroy()
	{

	}

	void update_start()
	{
		EntityMessage entity_message;
		while (queue_entities.try_pop(entity_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageAddObject && x)
				{
					debug_assert(!objects.contains(x.entity));
					objects.emplace<object_t>(x.entity, std::move(x.transform.pos), std::move(x.transform.quat));
				}
				void operator () (MessageRemove && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.remove(x.entity);
				}
				void operator () (MessageUpdateMovement && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.call(x.entity, update_movement{std::move(x.movement.vec)});
				}
				void operator () (MessageUpdateOrientationMovement && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.call(x.entity, update_orientation_movement{std::move(x.movement.quaternion)});
				}
				void operator () (MessageUpdateTransform && x)
				{
					debug_assert(objects.contains(x.entity));
					objects.call(x.entity, update_transform{std::move(x.transform)});
				}
			};
			visit(ProcessMessage{}, std::move(entity_message));
		}
	}

	void update_joints()
	{

	}
	void update_finish()
	{
		for (const auto & object : objects.get<object_t>())
		{
			auto matrix = make_translation_matrix(object.position) * make_matrix(object.orientation);
			engine::graphics::renderer::post_update_modelviewmatrix(objects.get_key(object), engine::graphics::data::ModelviewMatrix{std::move(matrix)});
		}
	}

	void post_add_object(engine::Entity entity, engine::transform_t && data)
	{
		const auto res = queue_entities.try_emplace(utility::in_place_type<MessageAddObject>, entity, std::move(data));
		debug_assert(res);
	}

	void post_remove(engine::Entity entity)
	{
		const auto res = queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity);
		debug_assert(res);
	}

	void post_update_movement(engine::Entity entity, movement_data && data)
	{
		const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateMovement>, entity, std::move(data));
		debug_assert(res);
	}

	void post_update_movement(const engine::Entity id, const transform_t translation)
	{

	}

	void post_update_orientation_movement(engine::Entity entity, orientation_movement && data)
	{
		const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateOrientationMovement>, entity, std::move(data));
		debug_assert(res);
	}

	void post_update_transform(engine::Entity entity, engine::transform_t && data)
	{
		const auto res = queue_entities.try_emplace(utility::in_place_type<MessageUpdateTransform>, entity, std::move(data));
		debug_assert(res);
	}
}
}
