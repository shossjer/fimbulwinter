
#ifndef ENGINE_GRAPHICS_VIEWER_HPP
#define ENGINE_GRAPHICS_VIEWER_HPP

#include "core/maths/util.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include "engine/Asset.hpp"
#include "engine/Entity.hpp"
#include "engine/MutableEntity.hpp"

#include <cstdint>

namespace engine
{
	namespace graphics
	{
		class renderer;
	}
}

namespace engine
{
	namespace graphics
	{
		class viewer
		{
		public:
			struct dynamic
			{
				engine::Asset parent;
				int slot;
			};
			struct fixed
			{
				engine::Asset parent;
				int slot;

				int width;
				int height;
			};

			struct horizontal
			{
				engine::Asset parent;
				int slot;
			};
			struct vertical
			{
				engine::Asset parent;
				int slot;
			};

			struct orthographic
			{
				double zNear;
				double zFar;
			};
			struct perspective
			{
				core::maths::radiand fovy;
				double zNear;
				double zFar;
			};

			struct camera
			{
				engine::Asset projection_3d;
				engine::Asset projection_2d;

				core::maths::Quaternionf rotation;
				core::maths::Vector3f translation;
			};
			struct projection
			{
				engine::Asset projection_3d;
				engine::Asset projection_2d;
			};
			struct rotate
			{
				core::maths::Quaternionf q;
			};
			struct rotation
			{
				core::maths::Quaternionf q;
			};
			struct translate
			{
				core::maths::Vector3f v;
			};
			struct translation
			{
				core::maths::Vector3f v;
			};

		public:
			~viewer();
			viewer(engine::graphics::renderer & renderer);
		};

		void post_add_frame(viewer & viewer, engine::Asset asset, viewer::dynamic && data);
		void post_add_frame(viewer & viewer, engine::Asset asset, viewer::fixed && data);
		void post_remove_frame(viewer & viewer, engine::Asset asset);

		void post_add_split(viewer & viewer, engine::Asset asset, viewer::horizontal && data);
		void post_add_split(viewer & viewer, engine::Asset asset, viewer::vertical && data);
		void post_remove_split(viewer & viewer, engine::Asset asset);

		void post_add_projection(viewer & viewer, engine::Asset asset, viewer::orthographic && data);
		void post_add_projection(viewer & viewer, engine::Asset asset, viewer::perspective && data);
		void post_remove_projection(viewer & viewer, engine::Asset asset);

		void post_add_camera(viewer & viewer, engine::MutableEntity entity, viewer::camera && data);
		void post_remove_camera(viewer & viewer, engine::Entity entity);
		void post_update_camera(viewer & viewer, engine::Entity entity, viewer::projection && data);
		void post_update_camera(viewer & viewer, engine::Entity entity, viewer::rotate && data);
		void post_update_camera(viewer & viewer, engine::Entity entity, viewer::rotation && data);
		void post_update_camera(viewer & viewer, engine::Entity entity, viewer::translate && data);
		void post_update_camera(viewer & viewer, engine::Entity entity, viewer::translation && data);

		void post_bind(viewer & viewer, engine::Asset frame, engine::Entity camera);
		void post_unbind(viewer & viewer, engine::Asset frame);
	}
}

#endif /* ENGINE_GRAPHICS_VIEWER_HPP */
