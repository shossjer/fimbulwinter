#include "core/container/Queue.hpp"
#include "core/container/Collection.hpp"
#include "core/container/ExchangeQueue.hpp"
#include "core/debug.hpp"
#include "core/maths/Matrix.hpp"
#include "core/maths/algorithm.hpp"

#include "engine/debug.hpp"
#include "engine/graphics/renderer.hpp"
#include "engine/graphics/viewer.hpp"
#include "engine/HashTable.hpp"

#include "utility/profiling.hpp"
#include "utility/variant.hpp"

static_hashes("root");

namespace
{
	engine::graphics::renderer * renderer = nullptr;

	struct dimension_t
	{
		int32_t width, height;
	};

	dimension_t dimension = {0, 0};

	// screen_coords = screen * projection * view * world_coords
	// world_coords = inv_view * inv_projection * inv_screen * screen_coords

	struct Viewport
	{
		int x;
		int y;
		int width;
		int height;

		engine::Token camera;

		Viewport(int x, int y, int width, int height, engine::Token camera)
			: x(x)
			, y(y)
			, width(width)
			, height(height)
			, camera(camera)
		{}
	};

	utility::heap_vector<engine::Token, Viewport> viewports;


	struct DynamicFrame
	{
		engine::Token camera;

		DynamicFrame(engine::graphics::viewer::dynamic &&)
		{}
	};
	struct FixedFrame
	{
		int width;
		int height;

		engine::Token camera;

		FixedFrame(engine::graphics::viewer::fixed && data)
			: width(data.width)
			, height(data.height)
		{}
	};

	struct Root
	{
		engine::Token node;
	};
	struct HorizontalSplit
	{
		engine::Token bottom;
		engine::Token top;

		HorizontalSplit(engine::graphics::viewer::horizontal &&)
		{}
	};
	struct VerticalSplit
	{
		engine::Token left;
		engine::Token right;

		VerticalSplit(engine::graphics::viewer::vertical &&)
		{}
	};

	core::container::UnorderedCollection
	<
		engine::Token,
		utility::static_storage_traits<83>,
		utility::static_storage<10, DynamicFrame>,
		utility::static_storage<10, FixedFrame>,
		utility::static_storage<1, Root>,
		utility::static_storage<10, HorizontalSplit>,
		utility::static_storage<10, VerticalSplit>
	>
	nodes;

	struct add_child
	{
		engine::Token child;
		int slot;

		void operator () (DynamicFrame &)
		{
			debug_unreachable();
		}
		void operator () (FixedFrame &)
		{
			debug_unreachable();
		}
		void operator () (Root & x)
		{
			debug_assert(x.node == engine::Hash{});
			x.node = child;
		}
		void operator () (HorizontalSplit & x)
		{
			switch (slot)
			{
			case 0:
				debug_assert(x.bottom == engine::Hash{});
				x.bottom = child;
				break;
			case 1:
				debug_assert(x.top == engine::Hash{});
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
				debug_assert(x.left == engine::Hash{});
				x.left = child;
				break;
			case 1:
				debug_assert(x.right == engine::Hash{});
				x.right = child;
				break;
			default:
				debug_unreachable();
			}
		}
	};

	struct bind_camera_to_frame
	{
		engine::Token camera;

		void operator () (DynamicFrame & x)
		{
			x.camera = camera;
		}
		void operator () (FixedFrame & x)
		{
			x.camera = camera;
		}
		void operator () (Root &)
		{
			debug_unreachable();
		}
		void operator () (HorizontalSplit &)
		{
			debug_unreachable();
		}
		void operator () (VerticalSplit &)
		{
			debug_unreachable();
		}
	};

	struct unbind_camera_from_frame
	{
		void operator () (DynamicFrame & x)
		{
			x.camera = engine::Token{};
		}
		void operator () (FixedFrame & x)
		{
			x.camera = engine::Token{};
		}
		void operator () (Root &)
		{
			debug_unreachable();
		}
		void operator () (HorizontalSplit &)
		{
			debug_unreachable();
		}
		void operator () (VerticalSplit &)
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
		engine::Token,
		utility::static_storage_traits<41>,
		utility::static_storage<10, Orthographic>,
		utility::static_storage<10, Perspective>
	>
	projections;

	struct extract_projection_matrices_2d
	{
		const Viewport & viewport;
		engine::graphics::data::camera_2d & data;

		void operator () (Orthographic & x)
		{
			data.projection = core::maths::Matrix4x4f::ortho(0.f, static_cast<float>(viewport.width),
			                                                 static_cast<float>(viewport.height), 0.f,
			                                                 x.zNear, x.zFar);
		}
		void operator () (Perspective &)
		{
			debug_fail();
		}
	};

	struct extract_projection_matrices_3d
	{
		const Viewport & viewport;
		engine::graphics::data::camera_3d & data;

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
		engine::Token projection_3d;
		engine::Token projection_2d;
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
		engine::Token,
		utility::static_storage_traits<41>,
		utility::static_storage<20, Camera>
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

		void operator () (Camera & x)
		{
			x.translation = data.v;
		}
	};

	struct extract_camera_matrices_2d
	{
		const Viewport & viewport;

		engine::graphics::data::camera_2d operator () (const Camera & x)
		{
			engine::graphics::data::camera_2d data;

			const auto projection_it = find(projections, x.projection_2d);
			if (debug_assert(projection_it != projections.end()))
			{
				projections.call(projection_it, extract_projection_matrices_2d{viewport, data});
			}
			else
			{
				data.projection = core::maths::Matrix4x4f::identity();
			}
			data.view = core::maths::Matrix4x4f::identity();
			return data;
		}
	};

	struct extract_camera_matrices_3d
	{
		const Viewport & viewport;

		engine::graphics::data::camera_3d operator () (const Camera & x)
		{
			engine::graphics::data::camera_3d data;

			const auto projection_it = find(projections, x.projection_3d);
			if (debug_assert(projection_it != projections.end()))
			{
				projections.call(projection_it, extract_projection_matrices_3d{viewport, data});
			}
			else
			{
				data.projection = core::maths::Matrix4x4f::identity();
				data.inv_projection = core::maths::Matrix4x4f::identity();
			}
			data.frame = core::maths::Matrix4x4f{
				static_cast<float>(viewport.width) / 2.f, 0.f, 0.f, /*viewport.x +*/ static_cast<float>(viewport.width) / 2.f,
				0.f, static_cast<float>(viewport.height) / -2.f, 0.f, /*viewport.y +*/ static_cast<float>(viewport.height) / 2.f,
				0.f, 0.f, 0.f, 0.f,
				0.f, 0.f, 0.f, 1.f
			};
			data.inv_frame = core::maths::Matrix4x4f{
				2.f / static_cast<float>(viewport.width), 0.f, 0.f, -(/*viewport.x * 2.f / viewport.width +*/ 1.f),
				0.f, -2.f / static_cast<float>(viewport.height), 0.f, 1.f/* - viewport.y * 2.f / viewport.height*/,
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

			void operator () (engine::Token asset, const DynamicFrame & node)
			{
				static_cast<void>(debug_verify(viewports.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(asset), std::forward_as_tuple(x, y, width, height, node.camera))));
			}
			void operator () (engine::Token asset, const FixedFrame & node)
			{
				static_cast<void>(debug_verify(viewports.try_emplace_back(std::piecewise_construct, std::forward_as_tuple(asset), std::forward_as_tuple(x, y, width, height, node.camera))));
			}
			void operator () (const Root & node)
			{
				const auto node_it = find(nodes, node.node);
				if (node_it != nodes.end())
				{
					nodes.call(node_it, *this);
				}
			}
			void operator () (const HorizontalSplit & node)
			{
				const int half_height = height / 2;

				const auto bottom_it = find(nodes, node.bottom);
				if (bottom_it != nodes.end())
				{
					nodes.call(bottom_it, BuildViewports{x, y + half_height, width, height - half_height});
				}

				const auto top_it = find(nodes, node.top);
				if (top_it != nodes.end())
				{
					nodes.call(top_it, BuildViewports{x, y, width, half_height});
				}
			}
			void operator () (const VerticalSplit & node)
			{
				const int half_width = width / 2;

				const auto left_it = find(nodes, node.left);
				if (left_it != nodes.end())
				{
					nodes.call(left_it, BuildViewports{x, y, half_width, height});
				}

				const auto right_it = find(nodes, node.right);
				if (right_it != nodes.end())
				{
					nodes.call(right_it, BuildViewports{x + half_width, y, width - half_width, height});
				}
			}
		};

		const auto root_it = find(nodes, engine::graphics::root_frame);
		if (debug_assert(root_it != nodes.end()))
		{
			nodes.call(root_it, BuildViewports{0, 0, dimension.width, dimension.height});
		}
	}
}

namespace
{
	struct MessageAddFrameDynamic
	{
		engine::Token asset;
		engine::graphics::viewer::dynamic data;
	};
	struct MessageAddFrameFixed
	{
		engine::Token asset;
		engine::graphics::viewer::fixed data;
	};
	struct MessageRemoveFrame
	{
		engine::Token asset;
	};

	struct MessageAddSplitHorizontal
	{
		engine::Token asset;
		engine::graphics::viewer::horizontal data;
	};
	struct MessageAddSplitVertical
	{
		engine::Token asset;
		engine::graphics::viewer::vertical data;
	};
	struct MessageRemoveSplit
	{
		engine::Token asset;
	};

	struct MessageAddProjectionOrthographic
	{
		engine::Token asset;
		engine::graphics::viewer::orthographic data;
	};
	struct MessageAddProjectionPerspective
	{
		engine::Token asset;
		engine::graphics::viewer::perspective data;
	};
	struct MessageRemoveProjection
	{
		engine::Token asset;
	};

	struct MessageAddCamera
	{
		engine::Token entity;
		engine::graphics::viewer::camera data;
	};
	struct MessageRemoveCamera
	{
		engine::Token entity;
	};
	struct MessageUpdateCameraProjection
	{
		engine::Token entity;
		engine::graphics::viewer::projection data;
	};
	struct MessageUpdateCameraRotate
	{
		engine::Token entity;
		engine::graphics::viewer::rotate data;
	};
	struct MessageUpdateCameraRotation
	{
		engine::Token entity;
		engine::graphics::viewer::rotation data;
	};
	struct MessageUpdateCameraTranslate
	{
		engine::Token entity;
		engine::graphics::viewer::translate data;
	};
	struct MessageUpdateCameraTranslation
	{
		engine::Token entity;
		engine::graphics::viewer::translation data;
	};

	struct MessageBind
	{
		engine::Token frame;
		engine::Token camera;
	};
	struct MessageUnbind
	{
		engine::Token frame;
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

	core::container::PageQueue<utility::heap_storage<Message>> queue_messages;
}

namespace engine
{
	namespace graphics
	{
		viewer::~viewer()
		{
			const auto root_it = find(nodes, engine::graphics::root_frame);
			if (debug_assert(root_it != nodes.end()))
			{
				nodes.erase(root_it);
			}

			engine::Token projections_not_unregistered[projections.max_size()];
			const auto projection_count = projections.get_all_keys(projections_not_unregistered, projections.max_size());
			debug_printline(engine::asset_channel, projection_count, " projections not unregistered:");
			for (auto i : ranges::index_sequence(projection_count))
			{
				debug_printline(engine::asset_channel, projections_not_unregistered[i]);
				static_cast<void>(i);
			}

			engine::Token nodes_not_unregistered[nodes.max_size()];
			const auto node_count = nodes.get_all_keys(nodes_not_unregistered, nodes.max_size());
			debug_printline(engine::asset_channel, node_count, " nodes not unregistered:");
			for (auto i : ranges::index_sequence(node_count))
			{
				debug_printline(engine::asset_channel, nodes_not_unregistered[i]);
				static_cast<void>(i);
			}

			::renderer = nullptr;
		}

		viewer::viewer(engine::graphics::renderer & renderer)
		{
			::renderer = &renderer;

			static_cast<void>(debug_verify(nodes.emplace<Root>(engine::graphics::root_frame, engine::Hash{})));
		}

		void update(viewer &)
		{
			profile_scope("viewer update");

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
						const auto camera_it = find(cameras, data.entity);
						if (camera_it != cameras.end())
						{
							if (!debug_assert(cameras.get_key(camera_it) < data.entity, "trying to add an older version camera"))
								return; // error

							cameras.erase(camera_it);
						}
						static_cast<void>(debug_verify(cameras.emplace<Camera>(data.entity, std::move(data.data))));
					}
					void operator () (MessageAddFrameDynamic && data)
					{
						const auto parent_it = find(nodes, data.data.parent);
						if (!debug_assert(parent_it != nodes.end()))
							return; // error

						nodes.call(parent_it, add_child{data.asset, data.data.slot});
						static_cast<void>(debug_verify(nodes.emplace<DynamicFrame>(data.asset, std::move(data.data))));
					}
					void operator () (MessageAddFrameFixed && data)
					{
						const auto parent_it = find(nodes, data.data.parent);
						if (!debug_assert(parent_it != nodes.end()))
							return; // error

						nodes.call(parent_it, add_child{data.asset, data.data.slot});
						static_cast<void>(debug_verify(nodes.emplace<FixedFrame>(data.asset, std::move(data.data))));
					}
					void operator () (MessageAddProjectionOrthographic && data)
					{
						static_cast<void>(debug_verify(projections.emplace<Orthographic>(data.asset, std::move(data.data))));
					}
					void operator () (MessageAddProjectionPerspective && data)
					{
						static_cast<void>(debug_verify(projections.replace<Perspective>(data.asset, std::move(data.data))));
					}
					void operator () (MessageAddSplitHorizontal && data)
					{
						const auto parent_it = find(nodes, data.data.parent);
						if (!debug_assert(parent_it != nodes.end()))
							return; // error

						nodes.call(parent_it, add_child{data.asset, data.data.slot});
						static_cast<void>(debug_verify(nodes.emplace<HorizontalSplit>(data.asset, std::move(data.data))));
					}
					void operator () (MessageAddSplitVertical && data)
					{
						const auto parent_it = find(nodes, data.data.parent);
						if (!debug_assert(parent_it != nodes.end()))
							return; // error

						nodes.call(parent_it, add_child{data.asset, data.data.slot});
						static_cast<void>(debug_verify(nodes.emplace<VerticalSplit>(data.asset, std::move(data.data))));
					}
					void operator () (MessageBind && data)
					{
						const auto frame_it = find(nodes, data.frame);
						if (!debug_assert(frame_it != nodes.end()))
							return; // error

						nodes.call(frame_it, bind_camera_to_frame{data.camera});
						rebuild_viewports = true;
					}
					void operator () (MessageRemoveCamera && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!debug_assert(camera_it != cameras.end()))
							return; // error

						cameras.erase(camera_it);
					}
					void operator () (MessageRemoveFrame && data)
					{
						const auto node_it = find(nodes, data.asset);
						if (!debug_assert(node_it != nodes.end()))
							return; // error

						nodes.erase(node_it);
					}
					void operator () (MessageRemoveProjection && data)
					{
						const auto projection_it = find(projections, data.asset);
						if (!debug_assert(projection_it != projections.end()))
							return; // error

						projections.erase(projection_it);
					}
					void operator () (MessageRemoveSplit && data)
					{
						const auto node_it = find(nodes, data.asset);
						if (!debug_assert(node_it != nodes.end()))
							return; // error

						nodes.erase(node_it);
					}
					void operator () (MessageUnbind && data)
					{
						const auto frame_it = find(nodes, data.frame);
						if (!debug_assert(frame_it != nodes.end()))
							return; // error

						nodes.call(frame_it, unbind_camera_from_frame{});
						rebuild_viewports = true;
					}
					void operator () (MessageUpdateCameraProjection && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!debug_assert(camera_it != cameras.end()))
							return; // error

						cameras.call(camera_it, update_camera_projection{std::move(data.data)});
						rebuild_matrices = true;
					}
					void operator () (MessageUpdateCameraRotate && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!debug_assert(camera_it != cameras.end()))
							return; // error

						cameras.call(camera_it, update_camera_rotate{std::move(data.data)});
						rebuild_matrices = true;
					}
					void operator () (MessageUpdateCameraRotation && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!debug_assert(camera_it != cameras.end()))
							return; // error

						cameras.call(camera_it, update_camera_rotation{std::move(data.data)});
						rebuild_matrices = true;
					}
					void operator () (MessageUpdateCameraTranslate && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!debug_assert(camera_it != cameras.end()))
							return; // error

						cameras.call(camera_it, update_camera_translate{std::move(data.data)});
						rebuild_matrices = true;
					}
					void operator () (MessageUpdateCameraTranslation && data)
					{
						const auto camera_it = find(cameras, data.entity);
						if (!/*debug_assert*/(camera_it != cameras.end()))
							return; // error

						cameras.call(camera_it, update_camera_translation{std::move(data.data)});
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
				for (const auto & viewport : viewports)
				{
					post_remove_display(*::renderer, viewport.first);
				}

				build_viewports();

				for (const auto && viewport : viewports)
				{
					const auto camera_it = find(cameras, viewport.second.camera);
					if (debug_assert(camera_it != cameras.end()))
					{
						post_add_display(
							*::renderer,
							viewport.first,
							engine::graphics::data::display{
								engine::graphics::data::viewport{
									viewport.second.x,
									viewport.second.y,
									viewport.second.width,
									viewport.second.height},
								cameras.call(camera_it, extract_camera_matrices_3d{viewport.second}),
								cameras.call(camera_it, extract_camera_matrices_2d{viewport.second})});
					}
				}
			}
			else if (rebuild_matrices)
			{
				for (const auto & viewport : viewports)
				{
					const auto camera_it = find(cameras, viewport.second.camera);
					if (debug_assert(camera_it != cameras.end()))
					{
						post_update_display(*::renderer, viewport.first, cameras.call(camera_it, extract_camera_matrices_3d{viewport.second}));
						post_update_display(*::renderer, viewport.first, cameras.call(camera_it, extract_camera_matrices_2d{viewport.second}));
					}
				}
			}
		}

		void post_add_frame(viewer &, engine::Token frame, viewer::dynamic && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddFrameDynamic>, frame, std::move(data)));
		}
		void post_add_frame(viewer &, engine::Token frame, viewer::fixed && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddFrameFixed>, frame, std::move(data)));
		}
		void post_remove_frame(viewer &, engine::Token frame)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageRemoveFrame>, frame));
		}

		void post_add_split(viewer &, engine::Token split, viewer::horizontal && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddSplitHorizontal>, split, std::move(data)));
		}
		void post_add_split(viewer &, engine::Token split, viewer::vertical && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddSplitVertical>, split, std::move(data)));
		}
		void post_remove_split(viewer &, engine::Token split)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageRemoveSplit>, split));
		}

		void notify_resize(viewer &, int width, int height)
		{
			debug_verify(queue_resize.try_push(width, height));
		}

		void post_add_projection(viewer &, engine::Token projection, viewer::orthographic && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddProjectionOrthographic>, projection, std::move(data)));
		}
		void post_add_projection(viewer &, engine::Token projection, viewer::perspective && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddProjectionPerspective>, projection, std::move(data)));
		}
		void post_remove_projection(viewer &, engine::Token projection)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageRemoveProjection>, projection));
		}

		void post_add_camera(viewer &, engine::Token camera, viewer::camera && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageAddCamera>, camera, std::move(data)));
		}
		void post_remove_camera(viewer &, engine::Token camera)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageRemoveCamera>, camera));
		}
		void post_update_camera(viewer &, engine::Token camera, viewer::projection && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraProjection>, camera, std::move(data)));
		}
		void post_update_camera(viewer &, engine::Token camera, viewer::rotate && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraRotate>, camera, std::move(data)));
		}
		void post_update_camera(viewer &, engine::Token camera, viewer::rotation && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraRotation>, camera, std::move(data)));
		}
		void post_update_camera(viewer &, engine::Token camera, viewer::translate && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraTranslate>, camera, std::move(data)));
		}
		void post_update_camera(viewer &, engine::Token camera, viewer::translation && data)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUpdateCameraTranslation>, camera, std::move(data)));
		}

		void post_bind(viewer &, engine::Token frame, engine::Token camera)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageBind>, frame, camera));
		}
		void post_unbind(viewer &, engine::Token frame)
		{
			debug_verify(queue_messages.try_emplace(utility::in_place_type<MessageUnbind>, frame));
		}
	}
}
