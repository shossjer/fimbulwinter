
#ifndef CORE_MATHS_MATRIX_HPP
#define CORE_MATHS_MATRIX_HPP

#include "util.hpp"

#include <utility/type_traits.hpp>

#include <algorithm>
#include <cmath>

namespace core
{
	namespace maths
	{
		namespace detail
		{
			template <typename T, typename Derived>
			class Matrix
			{
			private:
				using derived_type = Derived;

			public:
				/**
				 */
				static const derived_type & identity()
				{
					static derived_type matrix{T{1}, T{0}, T{0}, T{0},
					                           T{0}, T{1}, T{0}, T{0},
					                           T{0}, T{0}, T{1}, T{0},
					                           T{0}, T{0}, T{0}, T{1}};
					return matrix;
				}

				/**
				 * Because OpenGL 3 and later does not have this.
				 */
				static derived_type ortho(const T left,
				                          const T right,
				                          const T bottom,
				                          const T top,
				                          const T nearVal,
				                          const T farVal)
				{
					const T inv_width = T{1} / (right - left);
					const T inv_height = T{1} / (top - bottom);
					const T inv_depth = T{1} / (farVal - nearVal);

					return derived_type{T{2} * inv_width, T{0}, T{0}, -(right + left) * inv_width,
					                    T{0}, T{2} * inv_height, T{0}, -(top + bottom) * inv_height,
					                    T{0}, T{0}, T{-2} * inv_depth, -(farVal + nearVal) * inv_depth,
					                    T{0}, T{0}, T{0}, T{1}};
				}
				/**
				 * Because OpenGL 3 and later does not have this (actually it never did since it is a glu-thing).
				 */
				static derived_type perspective(const radian<T> fovy,
				                                const T aspect,
				                                const T zNear,
				                                const T zFar)
				{
					const T f = T{1} / std::tan(fovy.get() / T{2});
					const T inv_depth = T{1} / (zNear - zFar);

					return derived_type{f / aspect, T{0}, T{0}, T{0},
					                    T{0}, f, T{0}, T{0},
					                    T{0}, T{0}, (zFar + zNear) * inv_depth, T{2} * zFar * zNear * inv_depth,
					                    T{0}, T{0}, T{-1}, T{0}};
				}

				/**
				 */
				static derived_type rotation(const radian<T> radian, T x, T y, T z)
				{
					const T len_sq  = x * x + y * y + z * z;
					if (len_sq != T{1}) // TODO this will probably never be true
					{
						const T inv_len = T{1} / std::sqrt(len_sq);
						x *= inv_len;
						y *= inv_len;
						z *= inv_len;
					}
					const T c = std::cos(radian.get());
					const T s = std::sin(radian.get());

					return derived_type{x * x * (1 - c) + c    , x * y * (1 - c) - z * s, x * z * (1 - c) + y * s, T{0},
					                    y * x * (1 - c) + z * s, y * y * (1 - c) + c    , y * z * (1 - c) - x * s, T{0},
					                    z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, z * z * (1 - c) + c    , T{0},
					                    T{0}, T{0}, T{0}, T{1}};
				}
				/**
				 */
				static derived_type translation(const T x, const T y, const T z)
				{
					return derived_type{T{1}, T{0}, T{0}, x,
					                    T{0}, T{1}, T{0}, y,
					                    T{0}, T{0}, T{1}, z,
					                    T{0}, T{0}, T{0}, T{1}};
				}
			};
		}

		template <typename T>
		class Matrix : public detail::Matrix<T, Matrix<T>>
		{
		public:
			using array_type = T[4 * 4];
			using this_type = Matrix<T>;
			using value_type = T;

		private:
			array_type values;

		public:
			Matrix() = default;
			Matrix(const T m11, const T m12, const T m13, const T m14,
			       const T m21, const T m22, const T m23, const T m24,
			       const T m31, const T m32, const T m33, const T m34,
			       const T m41, const T m42, const T m43, const T m44)
			{
				this->set(m11, m12, m13, m14,
				          m21, m22, m23, m24,
				          m31, m32, m33, m34,
				          m41, m42, m43, m44);
			}

		public:
			this_type operator * (const this_type & m) const
			{
				this_type product;

				for (std::size_t column = 0; column < 4; ++column) // K
				{
					for (std::size_t row = 0; row < 4; ++row) // M
					{
						T dot = T{0};

						for (std::size_t i = 0; i < 4; ++i) // N
						{
							dot += this->values[row + i * 4] * m.values[i + column * 4]; // M N
						}
						product.values[row + column * 4] = dot; // M
					}
				}
				return product;
			}
			this_type & operator *= (const this_type & m)
			{
				return *this = *this * m;
			}

		public:
			array_type & get(array_type & buffer) const
			{
				std::copy(std::begin(this->values), std::end(this->values), std::begin(buffer));
				return buffer;
			}
			array_type & get_aligned(array_type & buffer) const
			{
				return this->get(buffer);
			}

			void set(const T m11, const T m12, const T m13, const T m14,
			         const T m21, const T m22, const T m23, const T m24,
			         const T m31, const T m32, const T m33, const T m34,
			         const T m41, const T m42, const T m43, const T m44)
			{
				this->values[ 0] = m11;
				this->values[ 1] = m21;
				this->values[ 2] = m31;
				this->values[ 3] = m41;
				this->values[ 4] = m12;
				this->values[ 5] = m22;
				this->values[ 6] = m32;
				this->values[ 7] = m42;
				this->values[ 8] = m13;
				this->values[ 9] = m23;
				this->values[10] = m33;
				this->values[11] = m43;
				this->values[12] = m14;
				this->values[13] = m24;
				this->values[14] = m34;
				this->values[15] = m44;
			}
			void set(const array_type & buffer)
			{
				std::copy(std::begin(buffer), std::end(buffer), std::begin(this->values));
			}
			void set_aligned(const array_type & buffer)
			{
				this->set(buffer);
			}
		};

		using Matrixd = Matrix<double>;
		using Matrixf = Matrix<float>;
	}
}

#endif /* CORE_MATHS_MATRIX_HPP */
