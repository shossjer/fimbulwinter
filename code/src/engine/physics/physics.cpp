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
		fiw_unused(debug_verify(components.emplace<Camera>(id, Camera{ bounded, position })));

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
		core::maths::Matrix4x4f matrix;

		explicit object_t(core::maths::Matrix4x4f matrix)
			: matrix(std::move(matrix))
		{}
	};

	core::container::Collection
	<
		engine::Token,
		utility::heap_storage_traits,
		utility::heap_storage<object_t>
	>
	objects;
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

	struct MessageSetTransformComponents
	{
		engine::Token entity;
		engine::physics::TransformComponents data;
	};

	using EntityMessage = utility::variant
	<
		MessageAddObject,
		MessageRemove,
		MessageSetTransformComponents
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
					debug_verify(objects.emplace<object_t>(x.entity, std::move(x.transform.matrix)));
				}

				void operator () (MessageRemove && x)
				{
					const auto object_it = find(objects, x.entity);
					if (debug_verify(object_it != objects.end()))
					{
						objects.erase(object_it);
					}
				}

				void operator () (MessageSetTransformComponents && x)
				{
					const auto handle = find(objects, x.entity);
					if (!debug_verify(handle != objects.end()))
						return;

					struct
					{
						engine::physics::TransformComponents && transform;

						void operator () (object_t & x)
						{
							debug_assert((transform.mask & ~(0x0001 | 0x0002 | 0x0004 | 0x0008 | 0x0010 | 0x0020)) == 0, "unknown mask");

							core::maths::Vector3f translation;
							core::maths::Matrix3x3f rotation;
							core::maths::Vector3f scale;
							core::maths::decompose(x.matrix, translation, rotation, scale);

							if (transform.mask & (0x0001 | 0x0002 | 0x0004))
							{
								core::maths::Vector3f::array_type buffer;
								translation.get(buffer);

								if (transform.mask & 0x0001)
								{
									buffer[0] = transform.values[0];
								}
								if (transform.mask & 0x0002)
								{
									buffer[1] = transform.values[1];
								}
								if (transform.mask & 0x0004)
								{
									buffer[2] = transform.values[2];
								}

								translation.set(buffer);
							}

							if (transform.mask & (0x0008 | 0x0010 | 0x0020))
							{
								core::maths::Matrix3x3f::array_type buffer;
								rotation.get(buffer);

								// XYZ rotation = Z(gamma) * Y(beta) * X(alpha)
								// cos b cos c  sin b sin a cos c - cos a sin c  sin b cos a cos c + sin a sin c
								// cos b sin c  sin b sin a sin c + cos a cos c  sin b cos a sin c - sin a cos c
								//   - sin b              cos b sin a                      cos b cos a

								const auto rxz = buffer[2];

								float alpha;
								float beta;
								float gamma;

								if (debug_assert(-1.f < rxz) &&
									debug_assert(rxz < 1.f))
								{
									const auto rxx = buffer[0];
									const auto rxy = buffer[1];
									const auto ryz = buffer[5];
									const auto rzz = buffer[8];

									alpha = std::atan2(ryz, rzz);
									beta = std::asin(-rxz);
									gamma = std::atan2(rxy, rxx);
								}
								else
								{
									// the matrix can be simplified to
									//  0   sin(a - c)    cos(a - c)
									//  0   cos(a - c)  - sin(a - c)
									// - 1      0             0
									// when rxz == - 1, and
									// 0  - sin(a + c)  - cos(a + c)
									// 0    cos(a + c)  - sin(a + c)
									// 1        0             0
									// when rxz == 1

									alpha = 0.f;
									beta = 0.f;
									gamma = 0.f;
								}

								if (transform.mask & 0x0008)
								{
									alpha = transform.values[3];
								}
								if (transform.mask & 0x0010)
								{
									beta = transform.values[4];
								}
								if (transform.mask & 0x0020)
								{
									gamma = transform.values[5];
								}

								const auto sa = std::sin(alpha);
								const auto ca = std::cos(alpha);
								const auto sb = std::sin(beta);
								const auto cb = std::cos(beta);
								const auto sc = std::sin(gamma);
								const auto cc = std::cos(gamma);

								rotation.set(
									cb * cc, sb * sa * cc - ca * sc, sb * ca * cc + sa * sc,
									cb * sc, sb * sa * sc + ca * cc, sb * ca * sc - sa * cc,
									  -sb  ,        cb * sa        ,        cb * ca        );
							}

							core::maths::compose(x.matrix, translation, rotation, scale);
						}
					}
					set_transform_components{std::move(x.data)};

					objects.call(handle, set_transform_components);
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
			post_update_modelviewmatrix(*::renderer, objects.get_key(object), engine::graphics::data::ModelviewMatrix{object.matrix});
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

	void set_transform_components(simulation & /*simulation*/, engine::Token entity, TransformComponents && data)
	{
		debug_verify(queue_entities.try_emplace(utility::in_place_type<MessageSetTransformComponents>, entity, std::move(data)));
	}
}
}
