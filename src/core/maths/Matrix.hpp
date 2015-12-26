
#ifndef CORE_MATHS_MATRIX_HPP
#define CORE_MATHS_MATRIX_HPP

#include "util.hpp"

#include <utility/type_traits.hpp>

#include <cmath>

namespace core
{
	namespace maths
	{
		template <typename T>
		class Matrix
		{
		public:
			/**
			 */
			using array_t = T[4 * 4];

		public:
			/**
			 * Because OpenGL 3 and later does not have this.
			 */
			static Matrix<T> ortho(const double left,
			                       const double right,
			                       const double bottom,
			                       const double top,
			                       const double nearVal,
			                       const double farVal)
			{
				const double inv_width = 1. / (right - left);
				const double inv_height = 1. / (top - bottom);
				const double inv_depth = 1. / (farVal - nearVal);

				const T m11 = T(2. * inv_width);
				const T m14 = T(-(right + left) * inv_width);
				const T m22 = T(2. * inv_height);
				const T m24 = T(-(top + bottom) * inv_height);
				const T m33 = T(-2. * inv_depth);
				const T m34 = T(-(farVal + nearVal) * inv_depth);

				return Matrix<T>(m11 , T{0}, T{0}, m14 ,
				                 T{0}, m22 , T{0}, m24 ,
				                 T{0}, T{0}, m33 , m34 ,
				                 T{0}, T{0}, T{0}, T{1});
			}
			/**
			 * Because OpenGL 3 and later does not have this (actually it never did since it is a glu-thing).
			 */
			static Matrix<T> perspective(const radiand fovy,
			                             const double aspect,
			                             const double zNear,
			                             const double zFar)
			{
				const double f = 1. / std::tan(fovy.get() / 2.);
				const double inv_depth = 1. / (zNear - zFar);

				const T m11 = T(f / aspect);
				const T m22 = T(f);
				const T m33 = T((zFar + zNear) * inv_depth);
				const T m34 = T(2. * zFar * zNear * inv_depth);

				return Matrix<T>(m11 , T{0}, T{0} , T{0},
				                 T{0}, m22 , T{0} , T{0},
				                 T{0}, T{0}, m33  , m34 ,
				                 T{0}, T{0}, T{-1}, T{0});
			}

		private:
			/**
			 */
			array_t values;

		public:
			/**
			 */
			Matrix() = default;
			/**
			 */
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
			 */
			const array_t & get() const
			{
				return this->values;
			}

		};

		using Matrixd = Matrix<double>;
		using Matrixf = Matrix<float>;
	}
}

#endif /* CORE_MATHS_MATRIX_HPP */
