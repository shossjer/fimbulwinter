#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/algorithm.hpp" // todo must be after "core/maths/Matrix.hpp"

#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/physics/physics.hpp"

#include "utility/variant.hpp"

#include <limits>

namespace
{
	engine::graphics::renderer * renderer = nullptr;
	engine::graphics::viewer * viewer = nullptr;

	typedef std::numeric_limits<float> lim;

	engine::physics::camera::Bounds bounds{
		{-lim::max(),-lim::max(),-lim::max() },
		{ lim::max(), lim::max(), lim::max() } };

	struct Camera
	{
		bool bounded;
		core::maths::Vector3f position;

		void update(core::maths::Vector3f & movement)
		{
			if (!this->bounded)
			{
				this->position += movement;
			}
			else
			{
				core::maths::Vector3f np = this->position + movement;

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

				this->position = core::maths::Vector3f{ pos[0], pos[1], pos[2] };
			}
		}
	};

	core::container::Collection
	<
		engine::Token,
		utility::static_storage_traits<23>,
		utility::static_storage<10, Camera>
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
	void set(simulation & simulation, Bounds && bounds_)
	{
		core::maths::Vector3f::array_type bmin;
		bounds_.min.get_aligned(bmin);
		core::maths::Vector3f::array_type bmax;
		bounds_.max.get_aligned(bmax);

		debug_assert(bmin[0] <= bmax[0]);
		debug_assert(bmin[1] <= bmax[1]);
		debug_assert(bmin[2] <= bmax[2]);

		::bounds = bounds_;

		// snap existing cameras to new bound
		for (Camera & camera : components.get<Camera>())
		{
			update(simulation, components.get_key(camera), core::maths::Vector3f{ 0.f, 0.f, 0.f });
		}
	}

	// TODO: make thread safe when needed
	void add(simulation & simulation, engine::Token id, core::maths::Vector3f position, bool bounded)
	{
		static_cast<void>(debug_verify(components.emplace<Camera>(id, Camera{ bounded, position })));

		// snap new camera to bounds
		update(simulation, id, core::maths::Vector3f{ 0.f, 0.f, 0.f });
	}

	// TODO: make thread safe when needed
	void update(simulation &, engine::Token id, core::maths::Vector3f movement)
	{
		const auto component_it = find(components, id);
		if (!debug_verify(component_it != components.end()))
			return; // error

		auto * const camera = components.get<Camera>(component_it);
		if (!debug_verify(camera))
			return; // error

		camera->update(movement);

		post_update_camera(
			*::viewer,
			id,
			engine::graphics::viewer::translation{camera->position});
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
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<object_t>
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
		engine::Token entity;
		engine::transform_t transform;
	};
	struct MessageRemove
	{
		engine::Token entity;
	};
	struct MessageUpdateMovement
	{
		engine::Token entity;
		engine::physics::movement_data movement;
	};
	struct MessageUpdateOrientationMovement
	{
		engine::Token entity;
		engine::physics::orientation_movement movement;
	};
	struct MessageUpdateTransform
	{
		engine::Token entity;
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

	core::container::PageQueue<utility::heap_storage<EntityMessage>> queue_entities;
}

namespace engine
{
namespace physics
{
	simulation::~simulation()
	{
		::viewer = nullptr;
		::renderer = nullptr;
	}

	simulation::simulation(engine::graphics::renderer & renderer_, engine::graphics::viewer & viewer_)
	{
		::renderer = &renderer_;
		::viewer = &viewer_;
	}

	void update_start(simulation &)
	{
		EntityMessage entity_message;
		while (queue_entities.try_pop(entity_message))
		{
			struct ProcessMessage
			{
				void operator () (MessageAddObject && x)
				{
					if (!debug_assert(find(objects, x.entity) != objects.end()))
						return; // error

					static_cast<void>(debug_verify(objects.emplace<object_t>(x.entity, std::move(x.transform.pos), std::move(x.transform.quat))));
				}
				void operator () (MessageRemove && x)
				{
					const auto object_it = find(objects, x.entity);
					if (!debug_assert(object_it != objects.end()))
						return; // error

					objects.erase(object_it);
				}
				void operator () (MessageUpdateMovement && x)
				{
					const auto object_it = find(objects, x.entity);
					if (!debug_assert(object_it != objects.end()))
						return; // error

					objects.call(object_it, update_movement{std::move(x.movement.vec)});
				}
				void operator () (MessageUpdateOrientationMovement && x)
				{
					const auto object_it = find(objects, x.entity);
					if (!debug_assert(object_it != objects.end()))
						return; // error

					objects.call(object_it, update_orientation_movement{std::move(x.movement.quaternion)});
				}
				void operator () (MessageUpdateTransform && x)
				{
					const auto object_it = find(objects, x.entity);
					if (!debug_assert(object_it != objects.end()))
						return; // error

					objects.call(object_it, update_transform{std::move(x.transform)});
				}
			};
			visit(ProcessMessage{}, std::move(entity_message));
		}
	}

	void update_joints(simulation &)
	{}

	void update_finish(simulation &)
	{
		for (const auto & object : objects.get<object_t>())
		{
			auto matrix = make_translation_matrix(object.position) * make_matrix(object.orientation);
			post_update_modelviewmatrix(*::renderer, objects.get_key(object), engine::graphics::data::ModelviewMatrix{std::move(matrix)});
		}
	}

	void post_add_object(simulation &, engine::Token entity, engine::transform_t && data)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageAddObject>, entity, std::move(data)));
	}

	void post_remove(simulation &, engine::Token entity)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageRemove>, entity));
	}

	void post_update_movement(simulation &, engine::Token entity, movement_data && data)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateMovement>, entity, std::move(data)));
	}

	void post_update_movement(simulation &, const engine::Token /*id*/, const transform_t /*translation*/)
	{}

	void post_update_orientation_movement(simulation &, engine::Token entity, orientation_movement && data)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateOrientationMovement>, entity, std::move(data)));
	}

	void post_update_transform(simulation &, engine::Token entity, engine::transform_t && data)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageUpdateTransform>, entity, std::move(data)));
	}
}
}
