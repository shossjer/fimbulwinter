
#ifndef ENGINE_GRAPHICS_OPENGL_MATRIX_HPP
#define ENGINE_GRAPHICS_OPENGL_MATRIX_HPP

#include <core/maths/Matrix.hpp>
#include <engine/graphics/opengl.hpp>

namespace core
{
	namespace maths
	{
		inline void glLoadMatrix(const Matrix4x4d & matrix)
		{
			// TODO: make aligned
			Matrix4x4d::array_type buffer;
			matrix.get_aligned(buffer);
			// glLoadMatrixd(buffer);
		}
		inline void glLoadMatrix(const Matrix4x4f & matrix)
		{
			// TODO: make aligned
			Matrix4x4f::array_type buffer;
			matrix.get_aligned(buffer);
			glLoadMatrixf(buffer);
		}
		/**
		 * \note Do not use, prefer glLoadMatrix instead!
		 */
		inline void glMultMatrix(const Matrix4x4d & matrix)
		{
			// TODO: make aligned
			Matrix4x4d::array_type buffer;
			matrix.get_aligned(buffer);
			// glMultMatrixd(buffer);
		}
		/**
		 * \note Do not use, prefer glLoadMatrix instead!
		 */
		inline void glMultMatrix(const Matrix4x4f & matrix)
		{
			// TODO: make aligned
			Matrix4x4f::array_type buffer;
			matrix.get_aligned(buffer);
			glMultMatrixf(buffer);
		}
	}
}

#endif /* ENGINE_GRAPHICS_OPENGL_MATRIX_HPP */
