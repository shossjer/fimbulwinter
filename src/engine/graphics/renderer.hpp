
#ifndef ENGINE_GRAPHICS_RENDERER_HPP
#define ENGINE_GRAPHICS_RENDERER_HPP

#include <core/maths/Matrix.hpp>
// #include <engine/graphics/opengl/Color.hpp>
// #include <engine/graphics/opengl/VertexBufferObject.hpp>

namespace engine
{
	namespace graphics
	{
		namespace renderer
		{
			void create();
			void destroy();
		}

		/**  */
		struct DrawArrays
		{
			core::maths::Matrixf matrix;
			// opengl::Color4f color;
			// GLuint vbo;
			// opengl::VertexBufferObject vbo;
		};
	}
}

#endif /* ENGINE_GRAPHICS_RENDERER_HPP */
