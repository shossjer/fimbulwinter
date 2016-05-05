
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include <core/maths/Matrix.hpp>
// #include <engine/graphics/opengl/Color.hpp>
// #include <engine/graphics/opengl/VertexBufferObject.hpp>

namespace engine
{
	namespace graphics
	{
		/**  */
		struct DrawArrays
		{
			core::maths::Matrix4x4f matrix;
			// opengl::Color4f color;
			// GLuint vbo;
			// opengl::VertexBufferObject vbo;
		};
		/**  */
		struct Point
		{
			core::maths::Matrix4x4f matrix;
			float size;
		};

		/**  */
		struct Rectangle
		{
			float red, green, blue;
			float width, height;
			float x, y, z; // offset from center
		};

		namespace renderer
		{
			void create();
			void destroy();

			template <typename T>
			void add(const std::size_t object, const T & component);
			void remove(std::size_t object);

			template <typename T>
			void set(const T & message);
		}
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
