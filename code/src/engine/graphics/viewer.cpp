
#include "viewer.hpp"

#include <core/container/CircleQueue.hpp>
#include <core/container/Collection.hpp>
#include <core/container/ExchangeQueue.hpp>
#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/Asset.hpp>
#include <engine/graphics/renderer.hpp>

#include "utility/variant.hpp"

namespace
{
	struct dimension_t
	{
		int32_t width, height;
	};

	dimension_t dimension = {0, 0};

	// screen_coords = screen * projection * view * world_coords
	// world_coords = inv_view * inv_projection * inv_screen * screen_coords

	struct Viewport
	{
		engine::Asset asset;

		int x;
		int y;
		int width;
		int height;

		engine::Entity camera;

		Viewport(engine::Asset asset, int x, int y, int width, int height, engine::Entity camera)
			: asset(asset)
			, x(x)
			, y(y)
			, width(width)
			, height(height)
			, camera(camera)
		{}
	};

	std::vector<Viewport> viewports;


	struct DynamicFrame
	{
		engine::Entity camera;

		DynamicFrame(engine::graphics::viewer::dynamic && data)
		{}
	};
	struct FixedFrame
	{
		int width;
		int height;

		engine::Entity camera;

		FixedFrame(engine::graphics::viewer::fixed && data)
			: width(data.width)
			, height(data.height)
		{}
	};

	struct Root
	{
		engine::Asset node;
	};
	struct HorizontalSplit
	{
		engine::Asset bottom;
		engine::Asset top;

		HorizontalSplit(engine::graphics::viewer::horizontal && data)
		{}
	};
	struct VerticalSplit
	{
		engine::Asset left;
		engine::Asset right;

		VerticalSplit(engine::graphics::viewer::vertical && data)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		83,
		std::array<DynamicFrame, 10>,
		std::array<FixedFrame, 10>,
		std::array<Root, 1>,
		std::array<HorizontalSplit, 10>,
		std::array<VerticalSplit, 10>
	>
	nodes;

	struct add_child
	{
		engine::Asset child;
		int slot;

		void operator () (DynamicFrame & x)
		{
			debug_unreachable();
		}
		void operator () (FixedFrame & x)
		{
			debug_unreachable();
		}
		void operator () (Root & x)
		{
			debug_assert(x.node == engine::Asset::null());
			x.node = child;
		}
		void operator () (HorizontalSplit & x)
		{
			switch (slot)
			{
			case 0:
				debug_assert(x.bottom == engine::Asset::null());
				x.bottom = child;
				break;
			case 1:
				debug_assert(x.top == engine::Asset::null());
				x.top = child;
				break;
			default:
				debug_unreachable();
			}
		}
		void operator () (VerticalSplit & x)
		{
			switch (slot)
			{
			case 0:
				debug_assert(x.left == engine::Asset::null());
				x.left = child;
				break;
			case 1:
				debug_assert(x.right == engine::Asset::null());
				x.right = child;
				break;
			default:
				debug_unreachable();
			}
		}
	};

	struct bind_camera_to_frame
	{
		engine::Entity camera;

		void operator () (DynamicFrame & x)
		{
			x.camera = camera;
		}
		void operator () (FixedFrame & x)
		{
			x.camera = camera;
		}
		void operator () (Root & x)
		{
			debug_unreachable();
		}
		void operator () (HorizontalSplit & x)
		{
			debug_unreachable();
		}
		void operator () (VerticalSplit & x)
		{
			debug_unreachable();
		}
	};

	struct unbind_camera_from_frame
	{
		void operator () (DynamicFrame & x)
		{
			x.camera = engine::Entity::null();
		}
		void operator () (FixedFrame & x)
		{
			x.camera = engine::Entity::null();
		}
		void operator () (Root & x)
		{
			debug_unreachable();
		}
		void operator () (HorizontalSplit & x)
		{
			debug_unreachable();
		}
		void operator () (VerticalSplit & x)
		{
			debug_unreachable();
		}
	};


	struct Orthographic
	{
		float zNear;
		float zFar;

		Orthographic(engine::graphics::viewer::orthographic && data)
			: zNear(static_cast<float>(data.zNear))
			, zFar(static_cast<float>(data.zFar))
		{}
	};
	struct Perspective
	{
		core::maths::radianf fovy;
		float zNear;
		float zFar;

		Perspective(engine::graphics::viewer::perspective && data)
			: fovy(static_cast<float>(data.fovy.get()))
			, zNear(static_cast<float>(data.zNear))
			, zFar(static_cast<float>(data.zFar))
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Asset,
		41,
		std::array<Orthographic, 10>,
		std::array<Perspective, 10>
	>
	projections;

	struct extract_projection_matrices_2d
	{
		const Viewport & viewport;
		engine::graphics::renderer::camera_2d & data;

		void operator () (Orthographic & x)
		{
			data.projection = core::maths::Matrix4x4f::ortho(0.f, static_cast<float>(viewport.width),
			                                                 static_cast<float>(viewport.height), 0.f,
			                                                 x.zNear, x.zFar);
		}
		void operator () (Perspective & x)
		{
			debug_fail();
		}
	};

	struct extract_projection_matrices_3d
	{
		const Viewport & viewport;
		engine::graphics::renderer::camera_3d & data;

		void operator () (Orthographic & x)
		{
			data.projection = core::maths::Matrix4x4f::ortho(0.f, static_cast<float>(viewport.width),
			                                                 static_cast<float>(viewport.height), 0.f,
			                                                 x.zNear, x.zFar);
		}
		void operator () (Perspective & x)
		{
			data.projection = core::maths::Matrix4x4f::perspective(x.fovy,
			                                                       static_cast<float>(static_cast<double>(viewport.width) / static_cast<double>(viewport.height)),
			                                                       x.zNear, x.zFar,
			                                                       data.inv_projection);
		}
	};


	struct Camera
	{
		engine::Asset projection_3d;
		engine::Asset projection_2d;
		core::maths::Quaternionf rotation;
		core::maths::Vector3f translation;

		Camera(engine::graphics::viewer::camera && data)
			: projection_3d(data.projection_3d)
			, projection_2d(data.projection_2d)
			, rotation(std::move(data.rotation))
			, translation(std::move(data.translation))
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

	struct update_camera_projection
	{
		engine::graphics::viewer::projection && data;

		void operator () (Camera & x)
		{
			x.projection_3d = data.projection_3d;
			x.projection_2d = data.projection_2d;
		}
	};

	struct update_camera_rotate
	{
		engine::graphics::viewer::rotate && data;

		void operator () (Camera & x)
		{
			x.rotation *= data.q;
		}
	};

	struct update_camera_translate
	{
		engine::graphics::viewer::translate && data;

		void operator () (Camera & x)
		{
			x.translation += data.v;
		}
	};

	struct update_camera_rotation
	{
		engine::graphics::viewer::rotation && data;

		void operator () (Camera & x)
		{
			x.rotation = data.q;
		}
	};

	struct update_camera_translation
	{
		engine::graphics::viewer::translation && data;

		void operator () (utility::monostate)
		{}
		void operator () (Camera & x)
		{
			x.translation = data.v;
		}
	};

	struct extract_camera_matrices_2d
	{
		const Viewport & viewport;

		engine::graphics::renderer::camera_2d operator () (const Camera & x)
		{
			engine::graphics::renderer::camera_2d data;
			projections.call(x.projection_2d, extract_projection_matrices_2d{viewport, data});
			data.view = core::maths::Matrix4x4f::identity();
			return data;
		}
	};

	struct extract_camera_matrices_3d
	{
		const Viewport & viewport;

		engine::graphics::renderer::camera_3d operator () (const Camera & x)
		{
			engine::graphics::renderer::camera_3d data;
			projections.call(x.projection_3d, extract_projection_matrices_3d{viewport, data});
			data.frame = core::maths::Matrix4x4f{
				viewport.width / 2.f, 0.f, 0.f, /*viewport.x +*/ viewport.width / 2.f,
				0.f, viewport.height / -2.f, 0.f, /*viewport.y +*/ viewport.height / 2.f,
				0.f, 0.f, 0.f, 0.f,
				0.f, 0.f, 0.f, 1.f
			};
			data.inv_frame = core::maths::Matrix4x4f{
				2.f / viewport.width, 0.f, 0.f, -(/*viewport.x * 2.f / viewport.width +*/ 1.f),
				0.f, -2.f / viewport.height, 0.f, 1.f/* - viewport.y * 2.f / viewport.height*/,
				0.f, 0.f, 0.f, 0.f,
				0.f, 0.f, 0.f, 1.f
			};
			data.view = make_matrix(conjugate(x.rotation));
			data.view.set_column(3, data.view * to_xyz1(-x.translation));
			data.inv_view = make_matrix(x.rotation);
			data.inv_view.set_column(3, data.inv_view * to_xyz1(x.translation));
			return data;
		}
	};


	void build_viewports()
	{
		viewports.clear();

		struct BuildViewports
		{
			int x;
			int y;
			int width;
			int height;

			void operator () (engine::Asset asset, const DynamicFrame & node)
			{
				viewports.emplace_back(asset, x, y, width, height, node.camera);
			}
			void operator () (engine::Asset asset, const FixedFrame & node)
			{
				viewports.emplace_back(asset, x, y, width, height, node.camera);
			}
			void operator () (const Root & node)
			{
				if (node.node != engine::Asset::null())
				{
					nodes.call(node.node, *this);
				}
			}
			void operator () (const HorizontalSplit & node)
			{
				const int half_height = height / 2;

				if (node.bottom != engine::Asset::null())
				{
					nodes.call(node.bottom, BuildViewports{x, y + half_height, width, height - half_height});
				}

				if (node.top != engine::Asset::null())
				{
					nodes.call(node.top, BuildViewports{x, y, width, half_height});
				}
			}
			void operator () (const VerticalSplit & node)
			{
				const int half_width = width / 2;

				if (node.left != engine::Asset::null())
				{
					nodes.call(node.left, BuildViewports{x, y, half_width, height});
				}

				if (node.right != engine::Asset::null())
				{
					nodes.call(node.right, BuildViewports{x + half_width, y, width - half_width, height});
				}
			}
		};

		nodes.call("root", BuildViewports{0, 0, dimension.width, dimension.height});
	}
}

namespace
{
	struct MessageAddFrameDynamic
	{
		engine::Asset asset;
		engine::graphics::viewer::dynamic data;
	};
	struct MessageAddFrameFixed
	{
		engine::Asset asset;
		engine::graphics::viewer::fixed data;
	};
	struct MessageRemoveFrame
	{
		engine::Asset asset;
	};

	struct MessageAddSplitHorizontal
	{
		engine::Asset asset;
		engine::graphics::viewer::horizontal data;
	};
	struct MessageAddSplitVertical
	{
		engine::Asset asset;
		engine::graphics::viewer::vertical data;
	};
	struct MessageRemoveSplit
	{
		engine::Asset asset;
	};

	struct MessageAddProjectionOrthographic
	{
		engine::Asset asset;
		engine::graphics::viewer::orthographic data;
	};
	struct MessageAddProjectionPerspective
	{
		engine::Asset asset;
		engine::graphics::viewer::perspective data;
	};
	struct MessageRemoveProjection
	{
		engine::Asset asset;
	};

	struct MessageAddCamera
	{
		engine::Entity entity;
		engine::graphics::viewer::camera data;
	};
	struct MessageRemoveCamera
	{
		engine::Entity entity;
	};
	struct MessageUpdateCameraProjection
	{
		engine::Entity entity;
		engine::graphics::viewer::projection data;
	};
	struct MessageUpdateCameraRotate
	{
		engine::Entity entity;
		engine::graphics::viewer::rotate data;
	};
	struct MessageUpdateCameraRotation
	{
		engine::Entity entity;
		engine::graphics::viewer::rotation data;
	};
	struct MessageUpdateCameraTranslate
	{
		engine::Entity entity;
		engine::graphics::viewer::translate data;
	};
	struct MessageUpdateCameraTranslation
	{
		engine::Entity entity;
		engine::graphics::viewer::translation data;
	};

	struct MessageBind
	{
		engine::Asset frame;
		engine::Entity camera;
	};
	struct MessageUnbind
	{
		engine::Asset frame;
	};

	using Message = utility::variant
	<
		MessageAddCamera,
		MessageAddFrameDynamic,
		MessageAddFrameFixed,
		MessageAddProjectionOrthographic,
		MessageAddProjectionPerspective,
		MessageAddSplitHorizontal,
		MessageAddSplitVertical,
		MessageBind,
		MessageRemoveCamera,
		MessageRemoveFrame,
		MessageRemoveProjection,
		MessageRemoveSplit,
		MessageUnbind,
		MessageUpdateCameraProjection,
		MessageUpdateCameraRotate,
		MessageUpdateCameraRotation,
		MessageUpdateCameraTranslate,
		MessageUpdateCameraTranslation
	>;

	core::container::ExchangeQueueSRSW<std::pair<int, int>> queue_resize;

	core::container::CircleQueueSRMW<Message, 50> queue_messages;
}

namespace engine
{
	namespace graphics
	{
		namespace viewer
		{
			void create()
			{
				nodes.emplace<Root>("root", engine::Asset::null());
			}

			void destroy()
			{}

			void update()
			{
				bool rebuild_viewports = false;
				bool rebuild_matrices = false;

				//
				// read messages
				//
				Message message;
				while (queue_messages.try_pop(message))
				{
					struct ProcessMessage
					{
						bool & rebuild_viewports;
						bool & rebuild_matrices;

						void operator () (MessageAddCamera && data)
						{
							cameras.emplace<Camera>(data.entity, std::move(data.data));
						}
						void operator () (MessageAddFrameDynamic && data)
						{
							debug_assert(nodes.contains(data.data.parent));
							nodes.call(data.data.parent, add_child{data.asset, data.data.slot});
							nodes.emplace<DynamicFrame>(data.asset, std::move(data.data));
						}
						void operator () (MessageAddFrameFixed && data)
						{
							debug_assert(nodes.contains(data.data.parent));
							nodes.call(data.data.parent, add_child{data.asset, data.data.slot});
							nodes.emplace<FixedFrame>(data.asset, std::move(data.data));
						}
						void operator () (MessageAddProjectionOrthographic && data)
						{
							projections.emplace<Orthographic>(data.asset, std::move(data.data));
						}
						void operator () (MessageAddProjectionPerspective && data)
						{
							projections.emplace<Perspective>(data.asset, std::move(data.data));
						}
						void operator () (MessageAddSplitHorizontal && data)
						{
							debug_assert(nodes.contains(data.data.parent));
							nodes.call(data.data.parent, add_child{data.asset, data.data.slot});
							nodes.emplace<HorizontalSplit>(data.asset, std::move(data.data));
						}
						void operator () (MessageAddSplitVertical && data)
						{
							debug_assert(nodes.contains(data.data.parent));
							nodes.call(data.data.parent, add_child{data.asset, data.data.slot});
							nodes.emplace<VerticalSplit>(data.asset, std::move(data.data));
						}
						void operator () (MessageBind && data)
						{
							nodes.call(data.frame, bind_camera_to_frame{data.camera});
							rebuild_viewports = true;
						}
						void operator () (MessageRemoveCamera && data)
						{
							cameras.remove(data.entity);
						}
						void operator () (MessageRemoveFrame && data)
						{
							nodes.remove(data.asset);
						}
						void operator () (MessageRemoveProjection && data)
						{
							projections.remove(data.asset);
						}
						void operator () (MessageRemoveSplit && data)
						{
							nodes.remove(data.asset);
						}
						void operator () (MessageUnbind && data)
						{
							nodes.call(data.frame, unbind_camera_from_frame{});
							rebuild_viewports = true;
						}
						void operator () (MessageUpdateCameraProjection && data)
						{
							cameras.call(data.entity, update_camera_projection{std::move(data.data)});
							rebuild_matrices = true;
						}
						void operator () (MessageUpdateCameraRotate && data)
						{
							cameras.call(data.entity, update_camera_rotate{std::move(data.data)});
							rebuild_matrices = true;
						}
						void operator () (MessageUpdateCameraRotation && data)
						{
							cameras.call(data.entity, update_camera_rotation{std::move(data.data)});
							rebuild_matrices = true;
						}
						void operator () (MessageUpdateCameraTranslate && data)
						{
							cameras.call(data.entity, update_camera_translate{std::move(data.data)});
							rebuild_matrices = true;
						}
						void operator () (MessageUpdateCameraTranslation && data)
						{
							cameras.try_call(data.entity, update_camera_translation{std::move(data.data)});
							rebuild_matrices = true;
						}
					};

					visit(ProcessMessage{rebuild_viewports, rebuild_matrices}, std::move(message));
				}

				//
				// read notifications
				//
				std::pair<int, int> notification_resize;
				if (queue_resize.try_pop(notification_resize))
				{
					dimension.width = notification_resize.first;
					dimension.height = notification_resize.second;
					rebuild_viewports = true;
				}

				//
				// write notifications
				//
				if (rebuild_viewports)
				{
					for (const Viewport & viewport : viewports)
					{
						engine::graphics::renderer::post_remove_display(viewport.asset);
					}

					build_viewports();

					for (const Viewport & viewport : viewports)
					{
						engine::graphics::renderer::post_add_display(viewport.asset, engine::graphics::renderer::display{engine::graphics::renderer::viewport{viewport.x, viewport.y, viewport.width, viewport.height}, cameras.call(viewport.camera, extract_camera_matrices_3d{viewport}), cameras.call(viewport.camera, extract_camera_matrices_2d{viewport})});
					}
				}
				else if (rebuild_matrices)
				{
					for (const Viewport & viewport : viewports)
					{
						engine::graphics::renderer::post_update_display(viewport.asset, cameras.call(viewport.camera, extract_camera_matrices_3d{viewport}));
						engine::graphics::renderer::post_update_display(viewport.asset, cameras.call(viewport.camera, extract_camera_matrices_2d{viewport}));
					}
				}
			}

			void post_add_frame(engine::Asset asset, dynamic && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddFrameDynamic>, asset, std::move(data));
				debug_assert(res);
			}
			void post_add_frame(engine::Asset asset, fixed && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddFrameFixed>, asset, std::move(data));
				debug_assert(res);
			}
			void post_remove_frame(engine::Asset asset)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRemoveFrame>, asset);
				debug_assert(res);
			}

			void post_add_split(engine::Asset asset, horizontal && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddSplitHorizontal>, asset, std::move(data));
				debug_assert(res);
			}
			void post_add_split(engine::Asset asset, vertical && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddSplitVertical>, asset, std::move(data));
				debug_assert(res);
			}
			void post_remove_split(engine::Asset asset)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRemoveSplit>, asset);
				debug_assert(res);
			}

			void notify_resize(int width, int height)
			{
				queue_resize.try_push(width, height);
			}

			void post_add_projection(engine::Asset asset, orthographic && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddProjectionOrthographic>, asset, std::move(data));
				debug_assert(res);
			}
			void post_add_projection(engine::Asset asset, perspective && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddProjectionPerspective>, asset, std::move(data));
				debug_assert(res);
			}
			void post_remove_projection(engine::Asset asset)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRemoveProjection>, asset);
				debug_assert(res);
			}

			void post_add_camera(engine::Entity entity, camera && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageAddCamera>, entity, std::move(data));
				debug_assert(res);
			}
			void post_remove_camera(engine::Entity entity)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageRemoveCamera>, entity);
				debug_assert(res);
			}
			void post_update_camera(engine::Entity entity, projection && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraProjection>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_camera(engine::Entity entity, rotate && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraRotate>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_camera(engine::Entity entity, rotation && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraRotation>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_camera(engine::Entity entity, translate && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraTranslate>, entity, std::move(data));
				debug_assert(res);
			}
			void post_update_camera(engine::Entity entity, translation && data)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraTranslation>, entity, std::move(data));
				debug_assert(res);
			}

			void post_bind(engine::Asset frame, engine::Entity camera)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageBind>, frame, camera);
				debug_assert(res);
			}
			void post_unbind(engine::Asset frame)
			{
				const auto res = queue_messages.try_emplace(utility::in_place_type<MessageUnbind>, frame);
				debug_assert(res);
			}
		}
	}
}
