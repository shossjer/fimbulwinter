
#ifndef ENGINE_GRAPHICS_OPENGL_MATRIX_HPP
#define ENGINE_GRAPHICS_OPENGL_MATRIX_HPP

#include <core/maths/Matrix.hpp>
#include <engine/graphics/opengl.hpp>

namespace core
{
	namespace maths
	{
		inline void glLoadMatrix(const Matrixd & matrix)
		{
			// TODO: make aligned
			Matrixd::array_type buffer;
			matrix.get_aligned(buffer);
			glLoadMatrixd(buffer);
		}
		inline void glLoadMatrix(const Matrixf & matrix)
		{
			// TODO: make aligned
			Matrixf::array_type buffer;
			matrix.get_aligned(buffer);
			glLoadMatrixf(buffer);
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_MATRIX_HPP */
