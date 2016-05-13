
#ifndef CORE_MATHS_MATRIX_HPP
#define CORE_MATHS_MATRIX_HPP

#include "util.hpp"

#include <utility/algorithm.hpp>
#include <utility/type_traits.hpp>

#include <algorithm>
#include <cmath>

namespace core
{
	namespace maths
	{
		template <std::size_t M, std::size_t N, typename T>
		class Matrix;
		template <std::size_t N, typename T>
		class Vector;

		template <std::size_t M, std::size_t N, typename T>
		Vector<M, T> operator * (const Matrix<M, N, T> & m, const Vector<N, T> & v);
	}
}

namespace core
{
	namespace maths
	{
		namespace detail
		{
			template <std::size_t M, std::size_t N, typename T, typename Derived>
			class Matrix
			{
			private:
				using derived_type = Derived;
			};
			template <typename T, typename Derived>
			class Matrix<2, 2, T, Derived>
			{
			private:
				using derived_type = Derived;

			public:
				/**
				 */
				static const derived_type & identity()
				{
					static derived_type matrix{T{1}, T{0},
					                           T{0}, T{1}};
					return matrix;
				}

				/**
				 */
				static derived_type rotation(const radian<T> radian)
				{
					const auto c = std::cos(radian.get());
					const auto s = std::sin(radian.get());

					return derived_type{c, -s,
					                    s,  c};
				}
			};
			template <typename T, typename Derived>
			class Matrix<4, 4, T, Derived>
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

		template <std::size_t M, std::size_t N, typename T>
		class Matrix : public detail::Matrix<M, N, T, Matrix<M, N, T>>
		{
		public:
			using array_type = T[M * N];
			using this_type = Matrix<M,  N, T>;
			using value_type = T;

			static constexpr std::size_t capacity = M * N;

		private:
			std::array<value_type, capacity> values;

		public:
			Matrix()// = default;
			{
			}
			template <typename ...Ps,
			          typename = mpl::enable_if_t<sizeof...(Ps) == capacity>>
			Matrix(Ps && ...ps) :
				values(utl::transpose<value_type, N, M>({{value_type{std::forward<Ps>(ps)}...}}))
			{
			}

		public:
			template <std::size_t K>
			Matrix<M, K, T> operator * (const Matrix<N, K, T> & m) const
			{
				Matrix<M, K, T> product;

				for (std::size_t column = 0; column < K; column++)
				{
					for (std::size_t row = 0; row < M; row++)
					{
						T dot = T{0};

						for (std::size_t i = 0; i < N; i++)
						{
							dot += this->values[row + i * M] * m.values[i + column * N];
						}
						product.values[row + column * M] = dot;
					}
				}
				return product;
			}
			this_type & operator *= (const Matrix<N, N, T> & m)
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

			template <typename ...Ps,
			          typename = mpl::enable_if_t<sizeof...(Ps) == capacity>>
			void set(Ps && ...ps)
			{
				this->values = utl::transpose<value_type, N, M>({{value_type{std::forward<Ps>(ps)}...}});
			}
			void set(const array_type & buffer)
			{
				std::copy(std::begin(buffer), std::end(buffer), std::begin(this->values));
			}
			void set_aligned(const array_type & buffer)
			{
				this->set(buffer);
			}

		public:
			template <std::size_t M_, std::size_t N_, typename T_>
			friend Vector<M_, T_> operator * (const Matrix<M_, N_, T_> & m, const Vector<N_, T_> & v);
		};

		using Matrix2x2d = Matrix<2, 2, double>;
		using Matrix2x2f = Matrix<2, 2, float>;
		using Matrix4x4d = Matrix<4, 4, double>;
		using Matrix4x4f = Matrix<4, 4, float>;
	}
}

#endif /* CORE_MATHS_MATRIX_HPP */
