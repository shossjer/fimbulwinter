
#ifndef CORE_MATHS_MATRIX_HPP
#define CORE_MATHS_MATRIX_HPP

#include <utility/type_traits.hpp>

#include <cmath>

namespace core
{
	namespace maths
	{
		template <typename T>
		class Matrix
		{
		private:
			// TODO: use vector types if available
			T values[4 * 4];

		public:
			Matrix() = default;
			Matrix(const T m11, const T m12, const T m13, const T m14,
			       const T m21, const T m22, const T m23, const T m24,
			       const T m31, const T m32, const T m33, const T m34,
			       const T m41, const T m42, const T m43, const T m44)
				{
					values[ 0] = m11;
					values[ 1] = m21;
					values[ 2] = m31;
					values[ 3] = m41;
					values[ 4] = m12;
					values[ 5] = m22;
					values[ 6] = m32;
					values[ 7] = m42;
					values[ 8] = m13;
					values[ 9] = m23;
					values[10] = m33;
					values[11] = m43;
					values[12] = m14;
					values[13] = m24;
					values[14] = m34;
					values[15] = m44;
				}

		public:
			/**
			 * Because OpenGL 3 and later does not have this.
			 */
			static Matrix<T> ortho(const T left,
			                       const T right,
			                       const T bottom,
			                       const T top,
			                       const T nearVal,
			                       const T farVal)
				{
					const T inv_width = 1. / (right - left);
					const T inv_height = 1. / (top - bottom);
					const T inv_depth = 1. / (farVal - nearVal);

					return Matrix<T>(2. * inv_width,  0, 0, -(right + left) * inv_width,
					                 0, 2. * inv_height, 0, -(top + bottom) * inv_height,
					                 0, 0, -2. * inv_depth, -(farVal + nearVal) * inv_depth,
					                 0, 0, 0, 1.);
				}
			/**
			 * Because OpenGL 3 and later does not have this (actually it never did since it is a glu-thing).
			 */
			static Matrix<T> perspective(const T fovy,
			                             const T aspect,
			                             const T zNear,
			                             const T zFar)
				{
					const T f = 1. / std::tan(fovy / 2.);
					const T inv_depth = 1. / (zNear - zFar);

					return Matrix<T>(f / aspect, 0, 0, 0,
					                 0, f, 0, 0,
					                 0, 0, (zFar + zNear) * inv_depth, 2 * zFar * zNear * inv_depth,
					                 0, 0, -1, 0);
				}
		};

		using Matrixd = Matrix<double>;
		using Matrixf = Matrix<float>;
	}
}

#endif /* CORE_MATHS_MATRIX_HPP */
