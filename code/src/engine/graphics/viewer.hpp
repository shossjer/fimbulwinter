
#ifndef ENGINE_GRAPHICS_VIEWER_HPP
#define ENGINE_GRAPHICS_VIEWER_HPP

#include <core/maths/util.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include "engine/Asset.hpp"
#include <engine/Entity.hpp>

#include <cstdint>

namespace engine
{
	namespace graphics
	{
		namespace viewer
		{
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
				engine::Asset projection;

				core::maths::Quaternionf rotation;
				core::maths::Vector3f translation;
			};
			struct projection
			{
				engine::Asset projection;
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

			void post_add_frame(engine::Asset asset, dynamic && data);
			void post_add_frame(engine::Asset asset, fixed && data);
			void post_remove_frame(engine::Asset asset);

			void post_add_split(engine::Asset asset, horizontal && data);
			void post_add_split(engine::Asset asset, vertical && data);
			void post_remove_split(engine::Asset asset);

			void notify_resize(int width, int height);

			void post_add_projection(engine::Asset asset, orthographic && data);
			void post_add_projection(engine::Asset asset, perspective && data);
			void post_remove_projection(engine::Asset asset);

			void post_add_camera(engine::Entity entity, camera && data);
			void post_remove_camera(engine::Entity entity);
			void post_update_camera(engine::Entity entity, projection && data);
			void post_update_camera(engine::Entity entity, rotate && data);
			void post_update_camera(engine::Entity entity, rotation && data);
			void post_update_camera(engine::Entity entity, translate && data);
			void post_update_camera(engine::Entity entity, translation && data);

			void post_bind(engine::Asset frame, engine::Entity camera);
			void post_unbind(engine::Asset frame);
		}
	}
}

#endif /* ENGINE_GRAPHICS_VIEWER_HPP */
