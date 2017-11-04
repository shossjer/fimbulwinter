
#include "viewer.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/Asset.hpp>
#include <engine/graphics/renderer.hpp>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			extern void notify(Camera2D && data);
			extern void notify(Camera3D && data);
			extern void notify(Viewport && data);
		}
	}
}

namespace
{
	struct dimension_t
	{
		int32_t width, height;
	};

	dimension_t dimension = {0, 0};

	engine::Entity active_2d = engine::Entity::null();
	engine::Entity active_3d = engine::Entity::null();

	// screen_coords = screen * projection * view * world_coords
	core::maths::Matrix4x4f screen = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f projection = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f view = core::maths::Matrix4x4f::identity();
	// world_coords = inv_view * inv_projection * inv_screen * screen_coords
	core::maths::Matrix4x4f inv_screen = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f inv_projection = core::maths::Matrix4x4f::identity();
	core::maths::Matrix4x4f inv_view = core::maths::Matrix4x4f::identity();

	struct Orthographic
	{
		float zNear;
		float zFar;

		core::maths::Matrix4x4f projection;

		Orthographic(engine::graphics::viewer::orthographic && data) :
			zNear(static_cast<float>(data.zNear)),
			zFar(static_cast<float>(data.zFar))
		{
		}

		Orthographic & operator = (engine::graphics::renderer::Camera2D & data)
		{
			data.projection = core::maths::Matrix4x4f::ortho(0.f, float(dimension.width),
			                                                 float(dimension.height), 0.f,
			                                                 zNear, zFar);
			return *this;
		}
		Orthographic & operator = (engine::graphics::renderer::Camera3D & data)
		{
			data.projection = core::maths::Matrix4x4f::ortho(0.f, float(dimension.width),
			                                                 float(dimension.height), 0.f,
			                                                 zNear, zFar);
			return *this;
		}
	};
	struct Perspective
	{
		core::maths::radianf fovy;
		float zNear;
		float zFar;

		Perspective(engine::graphics::viewer::perspective && data) :
			fovy(static_cast<float>(data.fovy.get())),
			zNear(static_cast<float>(data.zNear)),
			zFar(static_cast<float>(data.zFar))
		{
		}

		Perspective & operator = (engine::graphics::renderer::Camera3D & data)
		{
			data.projection = core::maths::Matrix4x4f::perspective(fovy,
			                                                       float(double(dimension.width) / double(dimension.height)),
			                                                       zNear, zFar,
			                                                       data.inv_projection);
			return *this;
		}
	};

	// core::container::UnorderedCollection
	// <
	// 	engine::extras::Asset,
	// 	41,
	// 	std::array<Orthographic, 10>,
	// 	std::array<Perspective, 10>
	// >
	// projections;
	Orthographic projection2D{engine::graphics::viewer::orthographic{-100., +100.}};
	Perspective projection3D{engine::graphics::viewer::perspective{core::maths::make_degree(80.), .125, 128.}};

	struct Camera
	{
		core::maths::Quaternionf rotation;
		core::maths::Vector3f translation;

		Camera(engine::graphics::viewer::camera && data) :
			rotation(std::move(data.rotation)),
			translation(std::move(data.translation))
		{}
	};

	core::container::Collection
	<
		engine::Entity,
		41,
		std::array<Camera, 20>,
		std::array<Camera, 1>
	>
	cameras;

	struct camera_rotate
	{
		engine::graphics::viewer::rotate && data;

		void operator () (Camera & x)
		{
			x.rotation *= data.q;
		}
	};

	struct camera_translate
	{
		engine::graphics::viewer::translate && data;

		void operator () (Camera & x)
		{
			x.translation += data.v;
		}
	};

	struct camera_set_rotation
	{
		engine::graphics::viewer::rotation && data;

		void operator () (Camera & x)
		{
			x.rotation = data.q;
		}
	};

	struct camera_set_translation
	{
		engine::graphics::viewer::translation && data;

		void operator () (Camera & x)
		{
			x.translation = data.v;
		}
	};

	struct camera_extract_2d
	{
		engine::graphics::renderer::Camera2D & data;

		void operator () (const Camera & x)
		{
			auto & matrix = data.view;
			matrix = make_matrix(x.rotation); // conjugate?
			matrix.set_column(3, matrix * to_xyz1(-x.translation));
		}
	};

	struct camera_extract_3d
	{
		engine::graphics::renderer::Camera3D & data;

		void operator () (const Camera & x)
		{
			auto & matrix = data.view;
			matrix = make_matrix(conjugate(x.rotation));
			matrix.set_column(3, matrix * to_xyz1(-x.translation));
			auto & inv_matrix = data.inv_view;
			inv_matrix = make_matrix(x.rotation);
			inv_matrix.set_column(3, to_xyz1(x.translation));
		}
	};
}

namespace
{
	core::container::ExchangeQueueSRSW<dimension_t> queue_dimension;

	// core::container::CircleQueueSRMW<std::pair<engine::Entity,
	//                                            engine::graphics::viewer::orthographic>,
	//                                  10> queue_add_orthographic;
	// core::container::CircleQueueSRMW<std::pair<engine::Entity,
	//                                            engine::graphics::viewer::perspective>,
	//                                  10> queue_add_perspective;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::viewer::camera>,
	                                 10> queue_add_camera;
	core::container::CircleQueueSRMW<engine::Entity,
	                                 10> queue_remove;
	core::container::ExchangeQueueSRMW<engine::Entity> queue_set_active_2d;
	core::container::ExchangeQueueSRMW<engine::Entity> queue_set_active_3d;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::viewer::rotate>,
	                                 10> queue_update_rotate;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::viewer::rotation>,
	                                 10> queue_update_rotation;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::viewer::translate>,
	                                 10> queue_update_translate;
	core::container::CircleQueueSRMW<std::pair<engine::Entity,
	                                           engine::graphics::viewer::translation>,
	                                 10> queue_update_translation;
}

namespace engine
{
	namespace graphics
	{
		namespace viewer
		{
			void update()
			{
				bool camera2d_has_changed = false;
				bool camera3d_has_changed = false;
				bool viewport_has_changed = false;
				//
				// read messages
				//
				// std::pair<engine::Entity,
				//           engine::graphics::viewer::orthographic> message_add_orthographic;
				// while (queue_add_orthographic.try_pop(message_add_orthographic))
				// {
				// 	components.add(message_add_orthographic.first,
				// 	               std::move(message_add_orthographic.second));
				// }
				// std::pair<engine::Entity,
				//           engine::graphics::viewer::perspective> message_add_perspective;
				// while (queue_add_perspective.try_pop(message_add_perspective))
				// {
				// 	components.add(message_add_perspective.first,
				// 	               std::move(message_add_perspective.second));
				// }
				std::pair<engine::Entity,
				          engine::graphics::viewer::camera> message_add_camera;
				while (queue_add_camera.try_pop(message_add_camera))
				{
					cameras.emplace<Camera>(message_add_camera.first,
					                        std::move(message_add_camera.second));
				}
				engine::Entity message_remove;
				while (queue_remove.try_pop(message_remove))
				{
					cameras.remove(message_remove);
				}
				engine::Entity message_set_active_2d;
				if (queue_set_active_2d.try_pop(message_set_active_2d))
				{
					if (active_2d != message_set_active_2d)
					{
						active_2d = message_set_active_2d;
						camera2d_has_changed = true;
					}
				}
				engine::Entity message_set_active_3d;
				if (queue_set_active_3d.try_pop(message_set_active_3d))
				{
					if (active_3d != message_set_active_3d)
					{
						active_3d = message_set_active_3d;
						camera3d_has_changed = true;
					}
				}
				std::pair<engine::Entity,
				          engine::graphics::viewer::rotate> message_update_rotate;
				while (queue_update_rotate.try_pop(message_update_rotate))
				{
					cameras.call(message_update_rotate.first,
					             camera_rotate{std::move(message_update_rotate.second)});
					if (message_update_rotate.first == active_2d) camera2d_has_changed = true;
					if (message_update_rotate.first == active_3d) camera3d_has_changed = true;
				}
				std::pair<engine::Entity,
				          engine::graphics::viewer::rotation> message_update_rotation;
				while (queue_update_rotation.try_pop(message_update_rotation))
				{
					cameras.call(message_update_rotation.first,
					             camera_set_rotation{std::move(message_update_rotation.second)});
					if (message_update_rotation.first == active_2d) camera2d_has_changed = true;
					if (message_update_rotation.first == active_3d) camera3d_has_changed = true;
				}
				std::pair<engine::Entity,
				          engine::graphics::viewer::translate> message_update_translate;
				while (queue_update_translate.try_pop(message_update_translate))
				{
					cameras.call(message_update_translate.first,
					             camera_translate{std::move(message_update_translate.second)});
					if (message_update_translate.first == active_2d) camera2d_has_changed = true;
					if (message_update_translate.first == active_3d) camera3d_has_changed = true;
				}
				std::pair<engine::Entity,
				          engine::graphics::viewer::translation> message_update_translation;
				while (queue_update_translation.try_pop(message_update_translation))
				{
					cameras.call(message_update_translation.first,
					             camera_set_translation{std::move(message_update_translation.second)});
					if (message_update_translation.first == active_2d) camera2d_has_changed = true;
					if (message_update_translation.first == active_3d) camera3d_has_changed = true;
				}
				//
				// read notifications
				//
				dimension_t notification_dimension;
				if (queue_dimension.try_pop(notification_dimension))
				{
					dimension = notification_dimension;
					camera2d_has_changed = true;
					camera3d_has_changed = true;
					viewport_has_changed = true;
				}
				//
				// write notifications
				//
				if (camera2d_has_changed)
				{
					engine::graphics::renderer::Camera2D data;
					projection2D = data; // ???
					if (active_2d == engine::Entity::null())
						data.view = core::maths::Matrix4x4f::identity();
					else
						cameras.call(active_2d, camera_extract_2d{data});
					notify(std::move(data));
				}
				if (camera3d_has_changed)
				{
					engine::graphics::renderer::Camera3D data;
					projection3D = data; // ???
					if (active_3d == engine::Entity::null())
					{
						data.view = core::maths::Matrix4x4f::identity();
						data.inv_view = core::maths::Matrix4x4f::identity();
					}
					else
						cameras.call(active_3d, camera_extract_3d{data});
					projection = data.projection;
					view = data.view;
					inv_projection = data.inv_projection;
					inv_view = data.inv_view;
					notify(std::move(data));
				}
				if (viewport_has_changed)
				{
					engine::graphics::renderer::Viewport data = {
						0,
						0,
						dimension.width,
						dimension.height
					};
					screen = core::maths::Matrix4x4f{
						data.width / 2.f, 0.f, 0.f, data.x + data.width / 2.f,
						0.f, data.height / -2.f, 0.f, data.y + data.height / 2.f,
						0.f, 0.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 1.f
					};
					inv_screen = core::maths::Matrix4x4f{
						2.f / data.width, 0.f, 0.f, -(data.x * 2.f / data.width + 1.f),
						0.f, -2.f / data.height, 0.f, 1.f - data.y * 2.f / data.height,
						0.f, 0.f, 0.f, 0.f,
						0.f, 0.f, 0.f, 1.f
					};
					notify(std::move(data));
				}
			}

			void notify_resize(const int width, const int height)
			{
				queue_dimension.try_push(width, height);
			}

			// void add(engine::Entity entity, orthographic && data)
			// {
			// 	const auto res = queue_add_orthographic.try_push(std::make_pair(entity, std::move(data)));
			// 	debug_assert(res);
			// }
			// void add(engine::Entity entity, perspective && data)
			// {
			// 	const auto res = queue_add_perspective.try_push(std::make_pair(entity, std::move(data)));
			// 	debug_assert(res);
			// }
			void add(engine::Entity entity, camera && data)
			{
				const auto res = queue_add_camera.try_push(std::make_pair(entity, std::move(data)));
				debug_assert(res);
			}
			void remove(engine::Entity entity)
			{
				const auto res = queue_remove.try_push(entity);
				debug_assert(res);
			}
			void set_active_2d(engine::Entity entity)
			{
				queue_set_active_2d.try_push(entity);
			}
			void set_active_3d(engine::Entity entity)
			{
				queue_set_active_3d.try_push(entity);
			}
			void update(engine::Entity entity, rotate && data)
			{
				const auto res = queue_update_rotate.try_push(std::make_pair(entity, std::move(data)));
				debug_assert(res);
			}
			void update(engine::Entity entity, rotation && data)
			{
				const auto res = queue_update_rotation.try_push(std::make_pair(entity, std::move(data)));
				debug_assert(res);
			}
			void update(engine::Entity entity, translate && data)
			{
				const auto res = queue_update_translate.try_push(std::make_pair(entity, std::move(data)));
				debug_assert(res);
			}
			void update(engine::Entity entity, translation && data)
			{
				const auto res = queue_update_translation.try_push(std::make_pair(entity, std::move(data)));
				debug_assert(res);
			}

			void from_screen_to_world(core::maths::Vector2f spos, core::maths::Vector3f & wpos)
			{
				wpos = to_xyz(inv_view * inv_projection * inv_screen * to_xy01(spos));
				// here we have a world position that sits on the near
				// plane of the  camera, so what we need to  do now is
				// to adjust the position so that it has z = 0
				const auto camerapos = to_xyz(inv_view.get_column<3>());
				const auto dir = wpos - camerapos;
				// camerapos_z + dir_z * t = 0  =>  t = -camerapos_z / dir_z
				core::maths::Vector3f::array_type cameraposbuffer;
				camerapos.get_aligned(cameraposbuffer);
				core::maths::Vector3f::array_type dirbuffer;
				dir.get_aligned(dirbuffer);
				const auto t = -cameraposbuffer[2] / dirbuffer[2];
				// wpos = camerapos + dir * t
				wpos = camerapos + dir * t;
			}
			void from_world_to_screen(core::maths::Vector3f wpos, core::maths::Vector2f & spos)
			{
				spos = to_xy(screen * projection * view * to_xyz1(wpos));
			}
		}
	}
}
