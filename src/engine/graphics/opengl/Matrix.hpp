
#ifndef ENGINE_GRAPHICS_OPENGL_MATRIX_HPP
#define ENGINE_GRAPHICS_OPENGL_MATRIX_HPP

#include <core/maths/Matrix.hpp>
#include <engine/graphics/opengl.hpp>

namespace core
{
	namespace maths
	{
		inline void glLoadMatrix(const Matrixd &matrix)
		{
			glLoadMatrixd(matrix.get());
		}
		inline void glLoadMatrix(const Matrixf &matrix)
		{
			glLoadMatrixf(matrix.get());
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_MATRIX_HPP */
