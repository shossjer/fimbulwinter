#pragma once

#include "core/debug.hpp"

#include "utility/algorithm.hpp"

namespace core
{
	namespace maths
	{
		template <std::size_t, std::size_t, typename>
		class Matrix;
		template <std::size_t, typename>
		class Plane;
		template <typename>
		class Quaternion;
		template <typename>
		class Scalar;
		template <std::size_t, typename>
		class Vector;
	}
}

namespace core
{
	namespace maths
	{
		////////////////////////////////////////////////////////////////////////
		//
		// various functions
		//
		////////////////////////////////////////////////////////////////////////
		/**
		 * Returns the square of `x`.
		 */
		template <typename T>
		inline auto square(T && x) -> decltype(x * x)
		{
			return x * x;
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  friend hack
		//
		////////////////////////////////////////////////////////////////////////
		struct algorithm
		{
			template <typename T>
			void decompose(const Matrix<4, 4, T> & m, Vector<3, T> & t, Quaternion<T> & r, Vector<3, T> & s)
			{
				// t
				const auto tx = m.values[12];
				const auto ty = m.values[13];
				const auto tz = m.values[14];
				t.set(tx, ty, tz);
				// s
				const auto xx = m.values[ 0];
				const auto xy = m.values[ 1];
				const auto xz = m.values[ 2];
				const auto yx = m.values[ 4];
				const auto yy = m.values[ 5];
				const auto yz = m.values[ 6];
				const auto zx = m.values[ 8];
				const auto zy = m.values[ 9];
				const auto zz = m.values[10];

				const auto sx = std::sqrt(xx * xx + xy * xy + xz * xz);
				const auto sy = std::sqrt(yx * yx + yy * yy + yz * yz);
				const auto sz = std::sqrt(zx * zx + zy * zy + zz * zz);
				s.set(sx, sy, sz);
				// r
				const auto rxx = xx / sx;
				const auto rxy = xy / sx;
				const auto rxz = xz / sx;
				const auto ryx = yx / sy;
				const auto ryy = yy / sy;
				const auto ryz = yz / sy;
				const auto rzx = zx / sz;
				const auto rzy = zy / sz;
				const auto rzz = zz / sz;
				// Quaternions, Ken Shoemake
				// http://www.cs.ucr.edu/~vbz/resources/quatut.pdf
				const auto trace = rxx + ryy + rzz;
				if (trace >= T{0})
				{
					const auto qw = std::sqrt((T{1} + trace) / T{4});
					const auto qw4 = T{4} * qw;
					const auto qx = (ryz - rzy) / qw4;
					const auto qy = (rzx - rxz) / qw4;
					const auto qz = (rxy - ryx) / qw4;
					r.set(qw, qx, qy, qz);
				}
				else
				{
					if (rxx >= ryy && rxx >= rzz)
					{
						const auto qx = std::sqrt((T{1} + rxx - ryy - rzz) / T{4});
						const auto qx4 = T{4} * qx;
						const auto qy = (rxy + ryx) / qx4;
						const auto qz = (rzx + rxz) / qx4;
						const auto qw = (ryz - rzy) / qx4;
						r.set(qw, qx, qy, qz);
					}
					else if (ryy >= rxx && ryy >= rzz)
					{
						const auto qy = std::sqrt((T{1} - rxx + ryy - rzz) / T{4});
						const auto qy4 = T{4} * qy;
						const auto qx = (rxy + ryx) / qy4;
						const auto qz = (ryz + rzy) / qy4;
						const auto qw = (rzx - rxz) / qy4;
						r.set(qw, qx, qy, qz);
					}
					else // rzz >= rxx && rzz >= ryy
					{
						const auto qz = std::sqrt((T{1} - rxx - ryy + rzz) / T{4});
						const auto qz4 = T{4} * qz;
						const auto qx = (rzx + rxz) / qz4;
						const auto qy = (ryz + rzy) / qz4;
						const auto qw = (rxy - ryx) / qz4;
						r.set(qw, qx, qy, qz);
					}
				}
			}

			template <std::size_t M, std::size_t N, typename T>
			void set_column_impl(Matrix<M, N, T> & m, std::size_t c, const Vector<M, T> & v)
			{
				debug_assert(c < N);
				for (std::size_t i = 0; i < M; i++)
				{
					m.values[M * c + i] = v.values[i];
				}
			}

			template <typename T>
			Vector<3, T> rotate(const Vector<3, T> & vec, const Quaternion<T> & q)
			{
				const Quaternion<T> qec{0.f, vec.values[0], vec.values[1], vec.values[2]};
				const auto pec = q * qec * conjugate(q);
				return {pec.values[1], pec.values[2], pec.values[3]};
			}

			template <typename T>
			Quaternion<T> rotation_of(const Vector<3, T> & vec)
			{
				if (vec.values[0] < T(-0.999)) // arbitrarily close to -1
					return {T{}, T{}, T{}, T{1}}; // any axis in the yz-place is okey

				const auto w2 = (vec.values[0] + 1) / 2;
				const auto w = std::sqrt(w2);

				const auto y = -vec.values[2] / (2 * w);
				const auto z = vec.values[1] / (2 * w);

				return {w, T{}, y, z};
			}
		};

		////////////////////////////////////////////////////////////////////////
		//
		//  member functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t M, std::size_t N, typename T>
		inline void Matrix<M, N, T>::set_column(std::size_t c, const Vector<M, T> & v)
		{
			algorithm{}.set_column_impl(*this, c, v);
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  operator functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t M, std::size_t N, typename T>
		inline Vector<M, T> operator * (const Matrix<M, N, T> & m, const Vector<N, T> & v)
		{
			Vector<M, T> product;

			for (std::size_t row = 0; row < M; row++)
			{
				T dot = T{0};

				for (std::size_t i = 0; i < N; i++)
				{
					dot += m.values[row + i * M] * v.values[i];
				}
				product.values[row] = dot;
			}
			return product;
		}

		////////////////////////////////////////////////////////////////////////
		//
		// function functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t D, typename T>
		Scalar<T> dot(const Plane<D, T> & plane, const Vector<D, T> & point);

		template <typename T>
		void decompose(const Matrix<4, 4, T> & m, Vector<3, T> & t, Quaternion<T> & r, Vector<3, T> & s)
		{
			algorithm{}.decompose(m, t, r, s);
		}

		template <std::size_t D, typename T>
		inline Scalar<T> distance(const Plane<D, T> & plane, const Vector<D, T> & point)
		{
			return dot(plane, point) + plane.values[D];
		}

		template <std::size_t D, typename T>
		inline Scalar<T> dot(const Plane<D, T> & plane, const Vector<D, T> & point)
		{
			T s{0};
			for (std::size_t i = 0; i < D; i++)
				s += plane.values[i] * point.values[i];
			return s;
		}

		template <std::size_t D, typename T>
		inline auto dot(const Vector<D, T> & point, const Plane<D, T> & plane) ->
			decltype(dot(plane, point))
		{
			return dot(plane, point);
		}

		template <typename T>
		inline Vector<3, T> rotate(const Vector<3, T> & vec, const Quaternion<T> & q)
		{
			return algorithm{}.rotate(vec, q);
		}

		template <typename T>
		inline Quaternion<T> rotation_of(const Vector<3, T> & vec)
		{
			return algorithm{}.rotation_of(vec);
		}

		////////////////////////////////////////////////////////////////////////
		//
		//  construction functions
		//
		////////////////////////////////////////////////////////////////////////
		template <std::size_t D, typename T>
		inline Plane<D, T> make_plane(const Vector<D, T> & point, const Vector<D, T> & normal)
		{
			return ext::apply(utl::constructor<Plane<D, T>>{}, utl::append(normal.values, dot(point, normal)));
		}
	}
}
