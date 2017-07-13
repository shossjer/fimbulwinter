
#include "physics.hpp"

#include <core/container/Collection.hpp>

#include <engine/graphics/viewer.hpp>

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

		engine::graphics::viewer::update(
			id,
			engine::graphics::viewer::translation{ camera.position });
	}
}
}
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

	}

	void update_joints()
	{

	}
	void update_finish()
	{

	}

	/**
	*	\note update Character or Dynamic object with delta movement or force.
	*/
	void post_update_movement(const engine::Entity id, const movement_data movement)
	{

	}

	/**
	*	\note update Kinematic object with position and rotation
	*/
	void post_update_movement(const engine::Entity id, const transform_t translation)
	{

	}
}
}
