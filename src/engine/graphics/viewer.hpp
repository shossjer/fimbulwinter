
#ifndef ENGINE_GRAPHICS_VIEWER_HPP
#define ENGINE_GRAPHICS_VIEWER_HPP

#include <core/maths/util.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>

#include <engine/Entity.hpp>

#include <cstdint>

namespace engine
{
	namespace graphics
	{
		namespace viewer
		{
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
				core::maths::Quaternionf rotation;
				core::maths::Vector3f translation;
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

			// void add(engine::Entity entity, orthographic && data);
			// void add(engine::Entity entity, perspective && data);
			void add(engine::Entity entity, camera && data);
			void remove(engine::Entity entity);
			void set_active_2d(engine::Entity entity);
			void set_active_3d(engine::Entity entity);
			void update(engine::Entity entity, rotate && data);
			void update(engine::Entity entity, rotation && data);
			void update(engine::Entity entity, translate && data);
			void update(engine::Entity entity, translation && data);

			/**
			 * Transforms screen coordinate to world coordinate.
			 *
			 * The origin in  screen space is located at  the top left
			 * corner of the window. X  increases towards the right, y
			 * increases downwards.
			 */
			void from_screen_to_world(core::maths::Vector2f spos, core::maths::Vector3f & wpos);
			/**
			 * Transforms world coordinate to screen coordinate.
			 *
			 * The origin in  screen space is located at  the top left
			 * corner of the window. X  increases towards the right, y
			 * increases downwards.
			 */
			void from_world_to_screen(core::maths::Vector3f wpos, core::maths::Vector2f & spos);
		}
	}
}

#endif /* ENGINE_GRAPHICS_VIEWER_HPP */
