#pragma once

#include "core/maths/util.hpp"
#include "core/maths/Quaternion.hpp"
#include "core/maths/Vector.hpp"

#include "engine/Asset.hpp"
#include "engine/Token.hpp"

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
				engine::Token parent;
				int slot;
			};
			struct fixed
			{
				engine::Token parent;
				int slot;

				int width;
				int height;
			};

			struct horizontal
			{
				engine::Token parent;
				int slot;
			};
			struct vertical
			{
				engine::Token parent;
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
				engine::Token projection_3d;
				engine::Token projection_2d;

				core::maths::Quaternionf rotation;
				core::maths::Vector3f translation;
			};
			struct projection
			{
				engine::Token projection_3d;
				engine::Token projection_2d;
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

		constexpr const auto root_frame = engine::Asset("root");

		void post_add_frame(viewer & viewer, engine::Token frame, viewer::dynamic && data);
		void post_add_frame(viewer & viewer, engine::Token frame, viewer::fixed && data);
		void post_remove_frame(viewer & viewer, engine::Token frame);

		void post_add_split(viewer & viewer, engine::Token split, viewer::horizontal && data);
		void post_add_split(viewer & viewer, engine::Token split, viewer::vertical && data);
		void post_remove_split(viewer & viewer, engine::Token split);

		void post_add_projection(viewer & viewer, engine::Token projection, viewer::orthographic && data);
		void post_add_projection(viewer & viewer, engine::Token projection, viewer::perspective && data);
		void post_remove_projection(viewer & viewer, engine::Token projection);

		void post_add_camera(viewer & viewer, engine::Token camera, viewer::camera && data);
		void post_remove_camera(viewer & viewer, engine::Token camera);
		void post_update_camera(viewer & viewer, engine::Token camera, viewer::projection && data);
		void post_update_camera(viewer & viewer, engine::Token camera, viewer::rotate && data);
		void post_update_camera(viewer & viewer, engine::Token camera, viewer::rotation && data);
		void post_update_camera(viewer & viewer, engine::Token camera, viewer::translate && data);
		void post_update_camera(viewer & viewer, engine::Token camera, viewer::translation && data);

		void post_bind(viewer & viewer, engine::Token frame, engine::Token camera);
		void post_unbind(viewer & viewer, engine::Token frame);
	}
}
